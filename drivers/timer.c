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