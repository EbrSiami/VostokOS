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
    __asm__ volatile(
        "pushfq\n"
        "pop %0\n"
        "cli\n"
        : "=r"(rflags)
    );
    lock->rflags = rflags; // save it

    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        __asm__ volatile("pause");
    }
}

void spinlock_release(spinlock_t* lock) {
    // 1. Release lock
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
    
    // 2. Re-enable interrupts
    // WARNING: In a real nested lock scenario, you'd check if IF was set before.
    // For now, we assume we always want interrupts back on.
    __asm__ volatile("sti");
}