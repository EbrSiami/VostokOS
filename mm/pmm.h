#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "../limine.h"

#define PAGE_SIZE 4096

// Initialize PMM with the memory map and the HHDM offset
void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset);

// Allocate a single physical page. Returns PHYSICAL address.
// Returns NULL (0) if out of memory.
void* pmm_alloc_page(void);

// Free a physical page.
void pmm_free_page(void* phys_addr);

// Allocate 'count' contiguous pages. Returns PHYSICAL address of the first page.
// Crucial for DMA and Framebuffers.
void* pmm_alloc_pages(size_t count);

// Free 'count' contiguous pages.
void pmm_free_pages(void* phys_addr, size_t count);

// Get memory statistics
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);
uint64_t pmm_get_total_memory(void);

#endif