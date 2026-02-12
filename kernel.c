#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "display/framebuffer.h"
#include "display/terminal.h"
#include "lib/printk.h"

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
    
    // Initialize framebuffer
    fb_init((uint32_t*)fb->address, fb->width, fb->height, fb->pitch, fb->bpp);
    
    // Initialize terminal
    terminal_init();
    
    // Test printk!
    printk("=== VostokOS Kernel ===\n\n");
    printk("Framebuffer: %dx%d @ %d bpp\n", fb->width, fb->height, fb->bpp);
    printk("Address: 0x%llx\n", (uint64_t)fb->address);
    printk("Pitch: %u bytes\n\n", fb->pitch);
    
    printk("Testing numbers:\n");
    printk("  Decimal: %d\n", 42);
    printk("  Hex (lower): 0x%x\n", 0xDEADBEEF);
    printk("  Hex (upper): 0x%X\n", 0xCAFEBABE);
    printk("  Pointer: %p\n", (void*)0x123456789ABC);
    
    printk("\nKernel ready!\n");
    
    hcf();
}