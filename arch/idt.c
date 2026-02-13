#include "idt.h"
#include "../lib/printk.h"
#include "../lib/string.h"

// IDT with 256 entries
static struct idt_entry idt[256];
static struct idt_ptr idt_pointer;

// External assembly function to load IDT
extern void idt_flush(uint64_t idt_ptr);

// Exception handler declarations (implemented in idt_asm.s)
extern void isr0(void);   // Divide by zero
extern void isr1(void);   // Debug
extern void isr2(void);   // NMI
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound range exceeded
extern void isr6(void);   // Invalid opcode
extern void isr7(void);   // Device not available
extern void isr8(void);   // Double fault
extern void isr9(void);   // Coprocessor segment overrun
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment not present
extern void isr12(void);  // Stack-segment fault
extern void isr13(void);  // General protection fault
extern void isr14(void);  // Page fault
extern void isr15(void);  // Reserved
extern void isr16(void);  // x87 FPU error
extern void isr17(void);  // Alignment check
extern void isr18(void);  // Machine check
extern void isr19(void);  // SIMD floating-point exception
extern void isr20(void);  // Virtualization exception
extern void isr21(void);  // Control protection exception
// 22-31 reserved
extern void isr31(void);

// Set an IDT entry
static void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    
    idt[num].selector = selector;
    idt[num].ist = 0;  // No IST for now
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

void idt_init(void) {
    idt_pointer.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_pointer.base = (uint64_t)&idt;
    
    // Clear IDT
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    
    // Set up exception handlers (ISRs 0-31)
    // Flags: 0x8E = Present, Ring 0, 64-bit Interrupt Gate
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint64_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E);
    
    // Load the IDT
    idt_flush((uint64_t)&idt_pointer);
    
    printk("[IDT] Interrupt Descriptor Table initialized\n");
}

// Exception handler names
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception"
};

// Common exception handler (called from assembly)
void isr_handler(uint64_t isr_number, uint64_t error_code) {
    printk("\n=== EXCEPTION ===\n");
    printk("Exception: %s (ISR %lld)\n", 
           isr_number < 22 ? exception_messages[isr_number] : "Unknown",
           isr_number);
    
    if (error_code != 0) {
        printk("Error Code: 0x%llx\n", error_code);
    }
    
    printk("\nSystem Halted.\n");
    
    // Halt
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}