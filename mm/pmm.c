#include "pmm.h"
#include "../lib/bitmap.h"
#include "../lib/string.h"
#include "../lib/printk.h"

// Global PMM state
static uint8_t *bitmap = NULL;
static uint64_t bitmap_size = 0;       // Size of bitmap in bytes
static uint64_t highest_page = 0;      // Highest physical page index
static uint64_t free_memory = 0;
static uint64_t used_memory = 0;
static uint64_t total_memory = 0;
static uint64_t hhdm_offset_global = 0;

// Helper to convert Physical Address to Virtual (HHDM)
static inline void* phys_to_virt(uint64_t phys) {
    return (void*)(phys + hhdm_offset_global);
}

void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset) {
    hhdm_offset_global = hhdm_offset;
    uint64_t highest_addr = 0;

    // 1. Calculate total memory and find the highest physical address
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        
        // Calculate top of this region
        uint64_t top = entry->base + entry->length;
        if (top > highest_addr) {
            highest_addr = top;
        }

        // Stats
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            total_memory += entry->length;
        }
    }

    highest_page = highest_addr / PAGE_SIZE;
    bitmap_size = (highest_page / 8) + 1;

    // 2. Find a place to store the bitmap
    // We need a contiguous chunk of memory large enough for the bitmap itself.
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            // Found a spot!
            // IMPORTANT: The bitmap address is Physical, we need to access it via Virtual (HHDM)
            bitmap = (uint8_t*)phys_to_virt(entry->base);
            
            // Initialize entire bitmap to 1 (Used)
            // We will selectively free usable regions later.
            memset(bitmap, 0xFF, bitmap_size);
            
            // Mark the region used by the bitmap itself as used (it's already 1, but for clarity)
            // We don't need to do anything special here because we just set everything to 1.
            break;
        }
    }

    if (bitmap == NULL) {
        printk("[PMM] Critical Error: Could not allocate PMM bitmap!\n");
        for(;;) __asm__("hlt");
    }

    // 3. Populate the bitmap based on the memory map
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        // We only care about USABLE memory. Everything else is already marked "Used" (0xFF).
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            // Iterate over pages in this region
            for (uint64_t addr = entry->base; addr < entry->base + entry->length; addr += PAGE_SIZE) {
                uint64_t page_idx = addr / PAGE_SIZE;
                
                // Ensure we don't free the memory where the bitmap ITSELF lives
                uint64_t bitmap_phys_start = (uint64_t)bitmap - hhdm_offset_global;
                uint64_t bitmap_phys_end = bitmap_phys_start + bitmap_size;

                if (addr >= bitmap_phys_start && addr < bitmap_phys_end) {
                    // This page holds the bitmap, keep it marked used
                    continue;
                }

                bitmap_clear(bitmap, page_idx);
                free_memory += PAGE_SIZE;
            }
        }
    }
    
    used_memory = total_memory - free_memory;
    printk("[PMM] Initialized. Bitmap size: %llu bytes. Free RAM: %llu MB\n", 
           bitmap_size, free_memory / 1024 / 1024);
}

// Allocate a single page
void* pmm_alloc_page(void) {
    // Simple first-fit search
    // Optimization TODO: Keep track of last_free_index to speed up search
    for (uint64_t i = 0; i < highest_page; i++) {
        if (!bitmap_test(bitmap, i)) {
            bitmap_set(bitmap, i);
            free_memory -= PAGE_SIZE;
            used_memory += PAGE_SIZE;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL; // OOM
}

// Free a single page
void pmm_free_page(void* phys_addr) {
    uint64_t addr = (uint64_t)phys_addr;
    uint64_t idx = addr / PAGE_SIZE;
    
    if (idx >= highest_page) return; // Out of bounds
    
    if (bitmap_test(bitmap, idx)) {
        bitmap_clear(bitmap, idx);
        free_memory += PAGE_SIZE;
        used_memory -= PAGE_SIZE;
    }
}

// Allocate contiguous pages (Crucial for GUI/DMA)
void* pmm_alloc_pages(size_t count) {
    if (count == 0) return NULL;

    for (uint64_t i = 0; i < highest_page; i++) {
        // Check if the first page is free
        if (!bitmap_test(bitmap, i)) {
            // Check if the next 'count' pages are also free
            int found = 1;
            for (size_t j = 1; j < count; j++) {
                if (i + j >= highest_page || bitmap_test(bitmap, i + j)) {
                    found = 0;
                    i += j; // Skip ahead
                    break;
                }
            }

            if (found) {
                // Mark all as used
                for (size_t j = 0; j < count; j++) {
                    bitmap_set(bitmap, i + j);
                }
                free_memory -= (count * PAGE_SIZE);
                used_memory += (count * PAGE_SIZE);
                return (void*)(i * PAGE_SIZE);
            }
        }
    }
    return NULL;
}

void pmm_free_pages(void* phys_addr, size_t count) {
    uint64_t start_addr = (uint64_t)phys_addr;
    uint64_t start_idx = start_addr / PAGE_SIZE;

    for (size_t i = 0; i < count; i++) {
        if (start_idx + i < highest_page) {
            bitmap_clear(bitmap, start_idx + i);
        }
    }
    free_memory += (count * PAGE_SIZE);
    used_memory -= (count * PAGE_SIZE);
}

uint64_t pmm_get_free_memory(void) { return free_memory; }
uint64_t pmm_get_used_memory(void) { return used_memory; }
uint64_t pmm_get_total_memory(void) { return total_memory; }