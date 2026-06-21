#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Structure representing the CPU registers saved on the stack during interrupts
struct interrupt_registers {
    // Registers pushed by common stub
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
    // Pushed by macro/CPU automatically
    uint64_t int_no, error_code;
    // Pushed automatically by hardware in 64-bit mode
    uint64_t rip, cs, rflags, rsp, ss;
};

// Function pointer for clean driver registration (Fixes Design Issue 6)
typedef uint64_t (*irq_handler_t)(uint64_t current_rsp);

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Initialize IDT
void idt_init(void);
void irq_register_handler(uint8_t irq_line, irq_handler_t handler);

#endif