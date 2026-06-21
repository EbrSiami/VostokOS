#include "idt.h"
#include "apic.h"
#include "../lib/panic.h"
#include "../lib/printk.h"
#include "../lib/string.h"

// Array of registered IRQ handlers (0-15)
static irq_handler_t irq_handlers[16] = {0};

static struct idt_entry idt[256];
static struct idt_ptr idt_pointer;

// Type-safe idt_flush declaration
extern void idt_flush(struct idt_ptr *ptr);

// External declarations for ISRs 0-31
#define DECLARE_ISR(num) extern void isr##num(void);
DECLARE_ISR(0)  DECLARE_ISR(1)  DECLARE_ISR(2)  DECLARE_ISR(3)
DECLARE_ISR(4)  DECLARE_ISR(5)  DECLARE_ISR(6)  DECLARE_ISR(7)
DECLARE_ISR(8)  DECLARE_ISR(9)  DECLARE_ISR(10) DECLARE_ISR(11)
DECLARE_ISR(12) DECLARE_ISR(13) DECLARE_ISR(14) DECLARE_ISR(15)
DECLARE_ISR(16) DECLARE_ISR(17) DECLARE_ISR(18) DECLARE_ISR(19)
DECLARE_ISR(20) DECLARE_ISR(21) DECLARE_ISR(22) DECLARE_ISR(23)
DECLARE_ISR(24) DECLARE_ISR(25) DECLARE_ISR(26) DECLARE_ISR(27)
DECLARE_ISR(28) DECLARE_ISR(29) DECLARE_ISR(30) DECLARE_ISR(31)

// External declarations for IRQs 0-15
#define DECLARE_IRQ(num) extern void irq##num(void);
DECLARE_IRQ(0)  DECLARE_IRQ(1)  DECLARE_IRQ(2)  DECLARE_IRQ(3)
DECLARE_IRQ(4)  DECLARE_IRQ(5)  DECLARE_IRQ(6)  DECLARE_IRQ(7)
DECLARE_IRQ(8)  DECLARE_IRQ(9)  DECLARE_IRQ(10) DECLARE_IRQ(11)
DECLARE_IRQ(12) DECLARE_IRQ(13) DECLARE_IRQ(14) DECLARE_IRQ(15)

static void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

// Function allowed for drivers to register themselves dynamically
void irq_register_handler(uint8_t irq_line, irq_handler_t handler) {
    if (irq_line < 16) {
        irq_handlers[irq_line] = handler;
    }
}

void idt_init(void) {
    idt_pointer.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_pointer.base = (uint64_t)(uintptr_t)&idt;
    
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    
    // Install Exception Handlers (0-21)
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);   idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E);   idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E);   idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E);   idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E);   idt_set_gate(9, (uint64_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E); idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E); idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E); idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E); idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E); idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E); idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E);
    
    // Install handlers for Intel-reserved exception vectors.
    // These vectors are currently treated as fatal faults.
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E); idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E); idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E); idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E); idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E);
    
    // Install IRQ Handlers (32-47)
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E);   idt_set_gate(33, (uint64_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint64_t)irq2, 0x08, 0x8E);   idt_set_gate(35, (uint64_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint64_t)irq4, 0x08, 0x8E);   idt_set_gate(37, (uint64_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint64_t)irq6, 0x08, 0x8E);   idt_set_gate(39, (uint64_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint64_t)irq8, 0x08, 0x8E);   idt_set_gate(41, (uint64_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint64_t)irq10, 0x08, 0x8E);  idt_set_gate(43, (uint64_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0x8E);  idt_set_gate(45, (uint64_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint64_t)irq14, 0x08, 0x8E);  idt_set_gate(47, (uint64_t)irq15, 0x08, 0x8E);

    idt_flush(&idt_pointer);
    printk("[IDT] Interrupt Descriptor Table initialized\n");
}

static const char *exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
    "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved Exception",
    "x87 FPU Error", "Alignment Check", "Machine Check", "SIMD Floating-Point Exception",
    "Virtualization Exception", "Control Protection Exception"
};

void isr_handler(struct interrupt_registers *regs) {
    if (regs->int_no == 3) {
        printk("[DEBUG] Breakpoint hit at RIP: 0x%016llx\n", regs->rip);
        return;
    }

    // find exception name
    const char *msg = (regs->int_no < 22) ? exception_messages[regs->int_no] : "Reserved/Unknown Exception";
    
    panic_exception(msg, regs);
}

uint64_t irq_handler(struct interrupt_registers *regs) {
    uint8_t actual_irq = regs->int_no - 32;
    uint64_t new_rsp = 0;

    if (actual_irq == 7 || actual_irq == 15) {
        // If it's a real IRQ, send EOI. If it's spurious, i safely return 0 without blocking lines.
        apic_send_eoi(); 
        return 0;
    }

    // Send APIC EOI *before* calling anything that can yield/switch context
    apic_send_eoi();

    // Call dynamic driver if one is registered
    if (irq_handlers[actual_irq] != 0) {
        new_rsp = irq_handlers[actual_irq](regs->rsp);
    }

    return new_rsp; 
}