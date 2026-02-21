#ifndef APIC_H
#define APIC_H

#include <stdint.h>
#include <stdbool.h>

// MSRs
#define IA32_APIC_BASE_MSR      0x1B
#define IA32_APIC_BASE_MSR_BSP  0x100
#define IA32_APIC_BASE_MSR_ENABLE 0x800

// LAPIC Registers (Offsets)
#define LAPIC_ID        0x0020  // Local APIC ID
#define LAPIC_VER       0x0030  // Local APIC Version
#define LAPIC_TPR       0x0080  // Task Priority Register
#define LAPIC_EOI       0x00B0  // End of Interrupt
#define LAPIC_SVR       0x00F0  // Spurious Interrupt Vector
#define LAPIC_ICR_LOW   0x0300  // Interrupt Command Register (Low)
#define LAPIC_ICR_HIGH  0x0310  // Interrupt Command Register (High)
#define LAPIC_LVT_TIMER 0x0320  // LVT Timer
#define LAPIC_LVT_LINT0 0x0350  // LVT LINT0
#define LAPIC_LVT_LINT1 0x0360  // LVT LINT1
#define LAPIC_LVT_ERROR 0x0370  // LVT Error
#define LAPIC_TICR      0x0380  // Timer Initial Count
#define LAPIC_TCCR      0x0390  // Timer Current Count
#define LAPIC_TDCR      0x03E0  // Timer Divide Configuration

// IOAPIC Registers
#define IOAPICID        0x00
#define IOAPICVER       0x01
#define IOAPICARB       0x02
#define IOREDTBL        0x10    

void apic_init(void);
void apic_send_eoi(void);
uint32_t apic_get_id(void);

#endif