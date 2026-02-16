#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// PIT ports
#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

// Initialize PIT timer
void timer_init(uint32_t frequency);

// Timer interrupt handler (called from IDT)
void timer_handler(void);

// Get ticks since boot
uint64_t timer_get_ticks(void);

// Get uptime in seconds
uint64_t timer_get_uptime(void);

#endif