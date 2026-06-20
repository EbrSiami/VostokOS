#include "gdt.h"
#include "../lib/panic.h"
#include "../lib/printk.h"

// 7 slots reserved, but we only advertise 5 to the CPU until TSS is configured
static struct gdt_entry gdt[7];
static struct gdt_ptr gdt_pointer;

// Type-safe declaration pointing directly to the struct
extern void gdt_flush(struct gdt_ptr *gdt_ptr_struct);

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
    // FIX: Set limit to only cover the first 5 valid entries (0 to 4).
    // This avoids advertising null slots (5-6) to the CPU prematurely.
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_pointer.base = (uint64_t)(uintptr_t)&gdt;
    
    // Null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Kernel code segment (64-bit)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);
    
    // Kernel data segment (64-bit)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xAF);
    
    // User code segment (64-bit)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF);
    
    // User data segment (64-bit)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xAF);
    
    // Explicitly zero future TSS descriptors for hygiene
    gdt_set_gate(5, 0, 0, 0, 0);
    gdt_set_gate(6, 0, 0, 0, 0);
    
    // FIX: Meaningful check on the actual internal values populated
    if (gdt_pointer.base == 0 || gdt_pointer.limit == 0) {
        // Assuming a panic or infinite loop mechanism here since it's an unrecoverable state
        panic("Invalid GDT pointer detected during initialization!"); // i'll use panic instead of while(1)
    }
    
    // Load the GDT using the type-safe pointer
    gdt_flush(&gdt_pointer);
    
    printk("[GDT] Global Descriptor Table initialized (5 entries active)\n");
}

// Future implementation snippet note:
// void tss_init() {
//      setup_tss_into_gdt_slots_5_and_6();
//      gdt_pointer.limit = (sizeof(struct gdt_entry) * 7) - 1; // expand limit here!
//      gdt_flush(&gdt_pointer);
//      load_tr(0x28);
// }