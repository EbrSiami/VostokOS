#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Get memory map from Limine
struct limine_memmap_response* get_memory_map(void);

// Calculate total and usable memory
uint64_t get_total_memory(void);
uint64_t get_usable_memory(void);

#endif