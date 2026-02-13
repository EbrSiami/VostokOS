#include "pic.h"
#include "../lib/printk.h"

// helper to write to I/O ports
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// helper to read from I/O ports
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// and a small delay for PIC operations
static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1, mask2;
    
    // Save current masks
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();
    
    // Set vector offsets (ICW2)
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    
    // Tell Master PIC there's a slave at IRQ2 (ICW3)
    outb(PIC1_DATA, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity (ICW3)
    outb(PIC2_DATA, 0x02);
    io_wait();
    
    // Set 8086 mode (ICW4)
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();
    
    // Restore saved masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
    
    printk("[PIC] Remapped IRQs: Master=0x%x, Slave=0x%x\n", offset1, offset2);
}

void pic_send_eoi(uint8_t irq) {
    // If IRQ came from slave PIC, send EOI to both
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send EOI to master
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_disable(void) {
    // Mask all interrupts on both PICs
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    printk("[PIC] Disabled\n");
}