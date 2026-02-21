#include "spinlock.h"

void spinlock_init(spinlock_t* lock) {
    lock->locked = 0;
    lock->holder_cpu = 0;
    uint64_t rip;
    __asm__ volatile("lea (%%rip), %0" : "=r"(rip));
    lock->holder_eip = (uint32_t)rip;
}

void spinlock_acquire(spinlock_t* lock) {
    uint64_t rflags;
    // Save current interrupt state and disable interrupts
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(rflags));
    
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        __asm__ volatile("pause");
    }

    // Store the state so release() knows whether to re-enable them
    lock->rflags = rflags;
}

void spinlock_release(spinlock_t* lock) {
    uint64_t rflags = lock->rflags;
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
    
    // Restore the interrupt state to exactly what it was before acquire()
    __asm__ volatile("push %0; popfq" :: "r"(rflags));
}