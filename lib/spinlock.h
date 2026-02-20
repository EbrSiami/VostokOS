#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t locked;
    uint64_t rflags;
    // For debugging: store who holds the lock
    uint32_t holder_cpu; 
    uint32_t holder_eip;
} spinlock_t;

void spinlock_init(spinlock_t* lock);
void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);

#endif