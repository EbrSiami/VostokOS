#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "display/framebuffer.h"
#include "display/terminal.h"

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
    
    // Test it out!
    terminal_write("Welcome to VostokOS!\n");
    terminal_write("Kernel loaded successfully.\n\n");
    terminal_write("System Information:\n");
    terminal_write("  - Framebuffer initialized\n");
    terminal_write("  - Terminal ready\n\n");
    terminal_write("Ready for development...\n");
    terminal_write("FUCK ISRAEL BTW");
    
    hcf();
}