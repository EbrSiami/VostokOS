#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "../lib/panic.h"
#include "../lib/spinlock.h"

// Header for each heap block
typedef struct block_header {
    size_t size;            // Size of the data part (excluding header)
    uint8_t is_free;        // 1 if free, 0 if used
    struct block_header* next;
    struct block_header* prev;
    uint64_t magic;         // Magic number to detect corruption
} block_header_t;

#define HEAP_MAGIC 0xC0FFEE1234567890
#define HEADER_SIZE sizeof(block_header_t)

// Global Heap State
static block_header_t* heap_start = NULL;
static block_header_t* heap_tail = NULL; // Optimization: Track the end
static uint64_t heap_current_end = 0;
static spinlock_t heap_lock;

// Helper: Align size to 16 bytes
static inline size_t align(size_t n) {
    return (n + 15) & ~15;
}

// Helper: Min function
static inline size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

// Core Logic: Find a free block, split it if necessary, and mark as used.
// Returns NULL if no block is found.
static block_header_t* find_split_and_alloc(size_t aligned_size) {
    block_header_t* current = heap_start;

    while (current != NULL) {
        if (current->magic != HEAP_MAGIC) {
            spinlock_release(&heap_lock);
            panic("Heap corruption detected (Magic Number Mismatch)!");
        }

        if (current->is_free && current->size >= aligned_size) {
            // Found a fit!
            
            // Split logic: Do we have enough space for a new header + 16 bytes?
            if (current->size >= aligned_size + HEADER_SIZE + 16) {
                block_header_t* new_block = (block_header_t*)((uint64_t)current + HEADER_SIZE + aligned_size);
                
                new_block->size = current->size - aligned_size - HEADER_SIZE;
                new_block->is_free = 1;
                new_block->magic = HEAP_MAGIC;
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) {
                    current->next->prev = new_block;
                } else {
                    // If current was tail, new_block is now tail
                    heap_tail = new_block;
                }
                
                current->next = new_block;
                current->size = aligned_size;
            }
            
            current->is_free = 0;
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Internal function: Expand the heap
// NOTE: Must be called with lock held!
static void heap_expand(size_t size_needed) {
    size_t total_needed = size_needed + HEADER_SIZE;
    
    // Enforce minimum expansion size
    if (total_needed < KHEAP_MIN_SIZE) {
        total_needed = KHEAP_MIN_SIZE;
    }

    size_t pages_needed = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t old_end = heap_current_end;

    for (size_t i = 0; i < pages_needed; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) {
            spinlock_release(&heap_lock);
            panic("Heap: OOM during expansion");
        }
        
        vmm_map(vmm_get_kernel_pml4(), heap_current_end, (uint64_t)phys, PTE_PRESENT | PTE_RW);
        heap_current_end += PAGE_SIZE;
    }

    // Create a new block in the newly mapped area
    block_header_t* new_block = (block_header_t*)old_end;
    new_block->size = (pages_needed * PAGE_SIZE) - HEADER_SIZE;
    new_block->is_free = 1;
    new_block->magic = HEAP_MAGIC;
    new_block->next = NULL;
    new_block->prev = heap_tail;
    
    // Link it
    if (heap_tail) {
        heap_tail->next = new_block;
    } else {
        // Should not happen if init was called, but for safety
        heap_start = new_block;
    }
    heap_tail = new_block;

    // Coalesce Left immediately (Merge with previous tail if it was free)
    if (new_block->prev && new_block->prev->is_free) {
        block_header_t* prev = new_block->prev;
        prev->size += HEADER_SIZE + new_block->size;
        prev->next = new_block->next; // which is NULL
        
        // Update tail pointer to the previous block (since it absorbed the new one)
        heap_tail = prev;
        
        // new_block is now invalid/consumed
    }
}

void kheap_init(void) {
    spinlock_init(&heap_lock);
    heap_current_end = KHEAP_START;
    
    // Allocate initial pages
    size_t pages = KHEAP_INITIAL_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) panic("Heap: Failed to allocate initial pages");
        
        vmm_map(vmm_get_kernel_pml4(), heap_current_end, (uint64_t)phys, PTE_PRESENT | PTE_RW);
        heap_current_end += PAGE_SIZE;
    }

    // Initialize first block
    heap_start = (block_header_t*)KHEAP_START;
    heap_start->size = KHEAP_INITIAL_SIZE - HEADER_SIZE;
    heap_start->is_free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    heap_start->magic = HEAP_MAGIC;
    
    heap_tail = heap_start;

    printk("[HEAP] Initialized. Lock ready.\n");
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    spinlock_acquire(&heap_lock);

    size_t aligned_size = align(size);
    
    // 1. Try to find a block
    block_header_t* block = find_split_and_alloc(aligned_size);
    
    if (block) {
        spinlock_release(&heap_lock);
        return (void*)((uint64_t)block + HEADER_SIZE);
    }
    
    // 2. No block found, expand heap
    heap_expand(aligned_size);
    
    // 3. Try again (Guaranteed to succeed unless OOM panic occurred)
    block = find_split_and_alloc(aligned_size);
    
    spinlock_release(&heap_lock);
    
    if (block) {
        return (void*)((uint64_t)block + HEADER_SIZE);
    }
    
    return NULL; // Should be unreachable
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    spinlock_acquire(&heap_lock);

    block_header_t* block = (block_header_t*)((uint64_t)ptr - HEADER_SIZE);
    
    if (block->magic != HEAP_MAGIC) {
        spinlock_release(&heap_lock);
        panic("Heap corruption detected during kfree!");
    }
    
    block->is_free = 1;
    
    // Coalesce Right
    if (block->next && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        
        // If the next block was the tail, this block becomes the tail
        if (block->next == heap_tail) {
            heap_tail = block;
        }
        
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    
    // Coalesce Left
    if (block->prev && block->prev->is_free) {
        block->prev->size += HEADER_SIZE + block->size;
        
        // If this block was the tail, the previous block becomes the tail
        if (block == heap_tail) {
            heap_tail = block->prev;
        }
        
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }

    spinlock_release(&heap_lock);
}

void* kcalloc(size_t num, size_t size) {
    // Overflow check
    if (size != 0 && num > UINT64_MAX / size) {
        return NULL;
    }

    size_t total = num * size;
    void* ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // We need to check the size without holding the lock yet, 
    // but accessing the header is safe as long as we own the pointer.
    block_header_t* block = (block_header_t*)((uint64_t)ptr - HEADER_SIZE);
    if (block->size >= new_size) return ptr; 
    
    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        // Safe copy size
        memcpy(new_ptr, ptr, min(block->size, new_size));
        kfree(ptr);
    }
    return new_ptr;
}

void kheap_print_stats(void) {
    // Note: iterating without lock is technically unsafe if other cores are active,
    // but for debug prints it's usually acceptable.
    block_header_t* current = heap_start;
    size_t free_bytes = 0;
    size_t used_bytes = 0;
    size_t free_blocks = 0;
    size_t used_blocks = 0;
    
    while (current) {
        if (current->is_free) {
            free_bytes += current->size;
            free_blocks++;
        } else {
            used_bytes += current->size;
            used_blocks++;
        }
        current = current->next;
    }
    
    printk("Heap Stats: Used: %llu bytes (%llu blocks) | Free: %llu bytes (%llu blocks)\n", 
           (uint64_t)used_bytes, (uint64_t)used_blocks, 
           (uint64_t)free_bytes, (uint64_t)free_blocks);
}