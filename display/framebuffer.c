#include "framebuffer.h"
#include "../lib/string.h"
#include "../mm/heap.h"
#include "../lib/printk.h"

static framebuffer_t fb;

void fb_init(uint32_t *addr, size_t width, size_t height, size_t pitch, uint16_t bpp) {
    fb.address = addr;

    fb.backbuffer = addr;

    fb.width = width;
    fb.height = height;
    fb.pitch = pitch;
    fb.bpp = bpp;
}

void fb_enable_double_buffering(void) {
    size_t buffer_size = fb.height * fb.pitch;
    
    fb.backbuffer = (uint32_t*)kmalloc(buffer_size);
    if (!fb.backbuffer) {
        printk("[FB] WARNING: Failed to allocate backbuffer. Falling back to MMIO.\n");
        fb.backbuffer = fb.address; 
    } else {
        memcpy(fb.backbuffer, fb.address, buffer_size);
        printk("[FB] Double buffering enabled successfully!\n");
    }
}

// Everything now writes to fb.backbuffer instead of fb.address
void fb_put_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= fb.width || y >= fb.height) {
        return;
    }
    
    size_t pixel_index = (y * (fb.pitch / 4)) + x;
    fb.backbuffer[pixel_index] = color;
}

void fb_clear(uint32_t color) {
    size_t total_pixels = fb.width * fb.height;
    
    if (color == 0) {
        memset(fb.backbuffer, 0, total_pixels * 4);
        return;
    }

    for (size_t i = 0; i < total_pixels; i++) {
        fb.backbuffer[i] = color;
    }
}

// Blast the RAM buffer to the GPU VRAM
void fb_swap(void) {
    if (fb.backbuffer == fb.address) return; // No backbuffer allocated

    // im not sure about this if compilers turn this into sse/avx instructions
    memcpy(fb.address, fb.backbuffer, fb.height * fb.pitch);
}

framebuffer_t* fb_get(void) {
    return &fb;
}