#include "panic.h"
#include "printk.h"

void panic_impl(const char *message, const char *file, int line) {
    // Disable interrupts immediately
    __asm__ volatile("cli");

    printk("\n\n");
    printk("****************************************\n");
    printk("KERNEL PANIC\n");
    printk("Message: %s\n", message);
    printk("File:    %s\n", file);
    printk("Line:    %d\n", line);
    printk("****************************************\n");
    printk("System Halted.\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}