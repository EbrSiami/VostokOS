#include "vmm.h"
#include "pmm.h"
#include "../lib/string.h"
#include "../lib/printk.h"

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

// Helper: Get the next level table. If it doesn't exist, allocate it.
// table_entry: Pointer to the entry in the current table (e.g., &pml4[idx])
// flags: Flags to use if we need to allocate a new table
static uint64_t* get_next_level(uint64_t* table_entry, uint64_t flags) {
    // Check if present flag (bit 0) is set
    if (!(*table_entry & PTE_PRESENT)) {
        // Not present, allocate a new table
        void* new_table_phys = pmm_alloc_page();
        if (!new_table_phys) {
            printk("[VMM] Critical: OOM during page table walk\n");
            for(;;) __asm__("hlt");
        }

        // Zero out the new table (Virtual access)
        void* new_table_virt = phys_to_virt((uint64_t)new_table_phys);
        memset(new_table_virt, 0, 4096);

        // Point the entry to the new table
        // We allow User/RW on intermediate tables, permissions are checked at the leaf (PT)
        *table_entry = (uint64_t)new_table_phys | PTE_PRESENT | PTE_RW | PTE_USER;
        
        return (uint64_t*)new_table_virt;
    }

    // Entry exists, mask out flags to get physical address
    uint64_t phys_addr = *table_entry & 0x000FFFFFFFFFF000;
    return (uint64_t*)phys_to_virt(phys_addr);
}

void vmm_map(uint64_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    // Calculate indices
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    // Walk the tables
    uint64_t* pdpt = get_next_level(&pml4[pml4_idx], flags);
    uint64_t* pd   = get_next_level(&pdpt[pdpt_idx], flags);
    uint64_t* pt   = get_next_level(&pd[pd_idx], flags);

    // Set the leaf entry
    pt[pt_idx] = paddr | flags;
    
    // Flush TLB
    tlb_flush(vaddr);
}

void vmm_unmap(uint64_t* pml4, uint64_t vaddr) {
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    // We don't use get_next_level here because we don't want to alloc if missing
    if (!(pml4[pml4_idx] & PTE_PRESENT)) return;
    
    uint64_t* pdpt = phys_to_virt(pml4[pml4_idx] & 0x000FFFFFFFFFF000);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return;

    uint64_t* pd = phys_to_virt(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000);
    if (!(pd[pd_idx] & PTE_PRESENT)) return;

    uint64_t* pt = phys_to_virt(pd[pd_idx] & 0x000FFFFFFFFFF000);
    
    // Clear the entry
    pt[pt_idx] = 0;
    tlb_flush(vaddr);
}

void vmm_init(void) {
    // 1. Allocate a new PML4 table
    void* new_pml4_phys = pmm_alloc_page();
    if (!new_pml4_phys) {
        printk("[VMM] Failed to allocate kernel PML4\n");
        for(;;) __asm__("hlt");
    }

    // 2. Get Virtual Address of new PML4
    kernel_pml4 = (uint64_t*)phys_to_virt((uint64_t)new_pml4_phys);
    memset(kernel_pml4, 0, 4096);

    // 3. Get the current (Limine) PML4
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
    
    // CR3 contains physical address, convert to virtual
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