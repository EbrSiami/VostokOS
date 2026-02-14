#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "display/framebuffer.h"
#include "display/terminal.h"
#include "lib/printk.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/pic.h"
#include "drivers/keyboard.h"
#include "shell/shell.h"

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static void hcf(void) {
    __asm__ ("cli");
    for (;;) {
        __asm__ ("hlt");
    }
}

void _start(void) {
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    
    // Initialize framebuffer and terminal
    fb_init((uint32_t*)fb->address, fb->width, fb->height, fb->pitch, fb->bpp);
    terminal_init();
    
    printk("=== VostokOS Kernel ===\n\n");
    
    // Initialize CPU structures
    gdt_init();
    idt_init();
    // Remap PIC (IRQs 0-15 → interrupts 32-47)
    pic_remap(32, 40);

    // Initialize keyboard
    keyboard_init();

    // Unmask keyboard IRQ (IRQ 1)
    pic_clear_mask(1);

    // Enable interrupts
    __asm__ volatile ("sti");

    printk("\n[KERNEL] Interrupts enabled!\n");
    printk("[KERNEL] System initialized successfully\n");
    
    shell_init();
    
    // Main loop - just wait for interrupts
    for (;;) {
        __asm__ volatile ("hlt");
    }
}