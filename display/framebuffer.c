#include "framebuffer.h"

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
    for (size_t y = 0; y < fb.height; y++) {
        for (size_t x = 0; x < fb.width; x++) {
            fb_put_pixel(x, y, color);
        }
    }
}

framebuffer_t* fb_get(void) {
    return &fb;
}