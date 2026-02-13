#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "display/framebuffer.h"
#include "display/terminal.h"
#include "lib/printk.h"
#include "arch/gdt.h"
#include "arch/idt.h"

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
    
    printk("\n[KERNEL] All systems initialized!\n\n");
    
    // Test divide by zero exception

    printk("Testing exception handling...\n");
    // int x = 5 / 0;  // This will trigger ISR 0
    
    printk("Kernel ready. Halting...\n");
    
    hcf();
}