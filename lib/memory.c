#include <stddef.h>
#include "memory.h"
#include "../limine.h"

uint64_t get_total_memory(void) {
    struct limine_memmap_response *memmap = get_memory_map();
    if (memmap == NULL) {
        return 0;
    }
    
    uint64_t total = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        total += entry->length;
    }
    
    return total;
}

uint64_t get_usable_memory(void) {
    struct limine_memmap_response *memmap = get_memory_map();
    if (memmap == NULL) {
        return 0;
    }
    
    uint64_t usable = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        
        // LIMINE_MEMMAP_USABLE = 0
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            usable += entry->length;
        }
    }
    
    return usable;
}