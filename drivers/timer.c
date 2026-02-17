#include "timer.h"
#include "../lib/printk.h"

static volatile uint64_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

// Helper to write to I/O port
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    timer_ticks = 0;
    
    // Calculate divisor
    uint32_t divisor = 1193182 / frequency;
    
    // Send command byte
    outb(PIT_COMMAND, 0x36);  // Channel 0, lobyte/hibyte, rate generator
    
    // Send divisor
    outb(PIT_CHANNEL0, divisor & 0xFF);         // Low byte
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  // High byte
    
    printk("[TIMER] PIT initialized at %u Hz\n", frequency);
}

void timer_handler(void) {
    timer_ticks++;
}

uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

uint64_t timer_get_uptime(void) {
    return timer_ticks / timer_frequency;
}

// get uptime in milliseconds (more precise!)
uint64_t timer_get_uptime_ms(void) {
    return (timer_ticks * 1000) / timer_frequency;
}

// sleep for specified milliseconds
void timer_sleep_ms(uint32_t ms) {
    uint64_t target_ticks = timer_ticks + (ms * timer_frequency) / 1000;
    
    // Busy wait (will be replaced with proper scheduling later)
    while (timer_ticks < target_ticks) {
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");  // Halt until next interrupt
    }
}

// sleep for specified seconds
void timer_sleep(uint32_t seconds) {
    timer_sleep_ms(seconds * 1000);
}

// wait for specified number of ticks
void timer_wait_ticks(uint64_t ticks) {
    uint64_t target = timer_ticks + ticks;
    while (timer_ticks < target) {
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
}

// get frequency
uint32_t timer_get_frequency(void) {
    return timer_frequency;
}