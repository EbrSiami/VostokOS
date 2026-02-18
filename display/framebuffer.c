#include "framebuffer.h"
#include "../lib/string.h"

static framebuffer_t fb;

void fb_init(uint32_t *addr, size_t width, size_t height, size_t pitch, uint16_t bpp) {
    fb.address = addr;
    fb.width = width;
    fb.height = height;
    fb.pitch = pitch;
    fb.bpp = bpp;
}

void fb_put_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= fb.width || y >= fb.height) {
        return;
    }
    
    size_t pixel_index = (y * (fb.pitch / 4)) + x;
    fb.address[pixel_index] = color;
}

void fb_clear(uint32_t color) {
    if (color == 0) {
        memset(fb.address, 0, fb.width * fb.height * 4);
        return;
    }

    size_t total_pixels = fb.width * fb.height;
    uint32_t *dest = (uint32_t *)fb.address;
    for (size_t i = 0; i < total_pixels; i++) {
        dest[i] = color;
    }
}

framebuffer_t* fb_get(void) {
    return &fb;
}