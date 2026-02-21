#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

// Page Table Entry Flags
#define PTE_PRESENT   (1ULL << 0)
#define PTE_RW        (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_PWT       (1ULL << 3)  // Page Write Through
#define PTE_PCD       (1ULL << 4)  // Page Cache Disable
#define PTE_NX        (1ULL << 63) // No Execute

// Initialize VMM (Create new PML4, copy kernel mappings, switch CR3)
void vmm_init(void);

// Map a virtual address to a physical address
// pml4: Virtual address of the PML4 table
// vaddr: Virtual address to map
// paddr: Physical address to map to
// flags: PTE flags (Present, RW, etc.)
void vmm_map(uint64_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags);

// Unmap a page
void vmm_unmap(uint64_t* pml4, uint64_t vaddr);

// Helper macro for MMIO mappings
#define PTE_MMIO      (PTE_PRESENT | PTE_RW | PTE_PCD | PTE_PWT | PTE_NX)

// Get the kernel's main PML4 table
uint64_t* vmm_get_kernel_pml4(void);

#endif