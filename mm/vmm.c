#include "vmm.h"
#include "pmm.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "../lib/panic.h"

// External variable from kernel.c
extern uint64_t hhdm_offset;

// The kernel's main PML4 table (Virtual Address)
static uint64_t* kernel_pml4 = NULL;

// Helper: Get Physical Address from Virtual (HHDM subtraction)
static inline uint64_t virt_to_phys(void* vaddr) {
    return (uint64_t)vaddr - hhdm_offset;
}

// Helper: Get Virtual Address from Physical (HHDM addition)
static inline void* phys_to_virt(uint64_t paddr) {
    return (void*)(paddr + hhdm_offset);
}

// Helper: Flush TLB for a specific address
static inline void tlb_flush(uint64_t vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

// Helper: Get the next level table. 
// If it doesn't exist, allocate it with 'flags'.
// If it DOES exist, ensure 'flags' are applied (e.g. upgrading Kernel entry to User).
static uint64_t* get_next_level(uint64_t* table_entry, uint64_t flags) {
    // 1. Check if the entry exists
    if (!(*table_entry & PTE_PRESENT)) {
        // Allocate new table
        void* new_table_phys = pmm_alloc_page();
        if (!new_table_phys) {
            panic("VMM: OOM during page table walk");
        }

        // Zero it out
        void* new_table_virt = phys_to_virt((uint64_t)new_table_phys);
        memset(new_table_virt, 0, 4096);

        // Set the entry
        *table_entry = (uint64_t)new_table_phys | flags;
        
        return (uint64_t*)new_table_virt;
    } 
    else {
        // 2. Entry exists: Check if we need to upgrade permissions
        if (flags & PTE_USER) {
            *table_entry |= PTE_USER;
        }
        
        // Ensure it's RW if requested
        if (flags & PTE_RW) {
            *table_entry |= PTE_RW;
        }

        // Return the virtual address of the next table
        uint64_t phys_addr = *table_entry & 0x000FFFFFFFFFF000;
        return (uint64_t*)phys_to_virt(phys_addr);
    }
}

void vmm_map(uint64_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    // Determine flags for intermediate tables.
    // They must be Present and RW. 
    uint64_t intermediate_flags = PTE_PRESENT | PTE_RW;
    if (flags & PTE_USER) {
        intermediate_flags |= PTE_USER;
    }

    // Walk the tables
    uint64_t* pdpt = get_next_level(&pml4[pml4_idx], intermediate_flags);
    uint64_t* pd   = get_next_level(&pdpt[pdpt_idx], intermediate_flags);
    uint64_t* pt   = get_next_level(&pd[pd_idx], intermediate_flags);

    // Check for double mapping
    if (pt[pt_idx] & PTE_PRESENT) {
        printk("[VMM] Error: VA 0x%llx already mapped to PA 0x%llx\n", 
               vaddr, pt[pt_idx] & 0x000FFFFFFFFFF000);
        panic("VMM: Double mapping detected");
    }

    // Set the leaf entry
    pt[pt_idx] = paddr | flags;
    
    tlb_flush(vaddr);
}

void vmm_unmap(uint64_t* pml4, uint64_t vaddr) {
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) return;
    
    uint64_t* pdpt = phys_to_virt(pml4[pml4_idx] & 0x000FFFFFFFFFF000);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return;

    uint64_t* pd = phys_to_virt(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000);
    if (!(pd[pd_idx] & PTE_PRESENT)) return;

    uint64_t* pt = phys_to_virt(pd[pd_idx] & 0x000FFFFFFFFFF000);
    
    // Clear the entry
    if (pt[pt_idx] & PTE_PRESENT) {
        pt[pt_idx] = 0;
        tlb_flush(vaddr);
    }
}

// Virtual to Physical Walker
uint64_t vmm_virt_to_phys(uint64_t* pml4, uint64_t vaddr) {
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) return 0;
    
    uint64_t* pdpt = phys_to_virt(pml4[pml4_idx] & 0x000FFFFFFFFFF000);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return 0;

    uint64_t* pd = phys_to_virt(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000);
    if (!(pd[pd_idx] & PTE_PRESENT)) return 0;

    // Check for 2MB Large Page (Bit 7)
    if (pd[pd_idx] & (1 << 7)) {
        // Mask bits 21-51 for base address
        uint64_t page_phys_base = pd[pd_idx] & 0x000FFFFFFFE00000; 
        // Offset is the lower 21 bits of vaddr
        return page_phys_base + (vaddr & 0x1FFFFF);
    }

    uint64_t* pt = phys_to_virt(pd[pd_idx] & 0x000FFFFFFFFFF000);
    if (!(pt[pt_idx] & PTE_PRESENT)) return 0;

    return (pt[pt_idx] & 0x000FFFFFFFFFF000) + (vaddr & 0xFFF);
}

void vmm_init(void) {
    // 1. Allocate a new PML4 table
    void* new_pml4_phys = pmm_alloc_page();
    if (!new_pml4_phys) {
        panic("VMM: Failed to allocate kernel PML4");
    }

    // 2. Get Virtual Address of new PML4
    kernel_pml4 = (uint64_t*)phys_to_virt((uint64_t)new_pml4_phys);
    memset(kernel_pml4, 0, 4096);

    // 3. Get the current (Limine) PML4
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
    
    uint64_t* old_pml4 = (uint64_t*)phys_to_virt(current_cr3 & 0x000FFFFFFFFFF000);

    // 4. Copy the Higher Half (Kernel + HHDM)
    // Entries 256 to 511 cover 0xffff800000000000 to 0xffffffffffffffff
    for (int i = 256; i < 512; i++) {
        kernel_pml4[i] = old_pml4[i];
    }

    // 5. Switch to the new PML4
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint64_t)new_pml4_phys) : "memory");

    printk("[VMM] Initialized. CR3 switched to new PML4 at 0x%llx\n", (uint64_t)new_pml4_phys);
}

uint64_t* vmm_get_kernel_pml4(void) {
    return kernel_pml4;
}