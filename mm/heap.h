#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

// Heap configuration
#define KHEAP_START         0xffffa00000000000
#define KHEAP_INITIAL_SIZE  (1024 * 1024) // Start with 1MB
#define KHEAP_MIN_SIZE      (4096)        // Minimum expansion size

// Initialize the kernel heap
void kheap_init(void);

// Allocate memory
void* kmalloc(size_t size);

// Allocate memory cleared to zero
void* kcalloc(size_t num, size_t size);

// Reallocate memory (resize)
void* krealloc(void* ptr, size_t new_size);

// Free memory
void kfree(void* ptr);

// Debug: Print heap status
void kheap_print_stats(void);

#endif