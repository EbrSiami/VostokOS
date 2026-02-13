#include "gdt.h"
#include "../lib/printk.h"

// GDT entries
static struct gdt_entry gdt[5];
static struct gdt_ptr gdt_pointer;

// External assembly function to load GDT
extern void gdt_flush(uint64_t gdt_ptr);

// Set a GDT entry
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    
    gdt[num].access = access;
}

void gdt_init(void) {
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_pointer.base = (uint64_t)&gdt;
    
    // Null descriptor (required)
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Kernel code segment (64-bit)
    // Base = 0, Limit = 0xFFFFF
    // Access: Present, Ring 0, Code segment, Executable, Readable
    // Granularity: 64-bit, Page granularity
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);
    
    // Kernel data segment (64-bit)
    // Access: Present, Ring 0, Data segment, Writable
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // User code segment (64-bit) - for later
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF);
    
    // User data segment (64-bit) - for later
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // Load the GDT
    gdt_flush((uint64_t)&gdt_pointer);
    
    printk("[GDT] Global Descriptor Table initialized\n");
}