#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t *address;
    size_t width;
    size_t height;
    size_t pitch;
    uint16_t bpp;
} framebuffer_t;

void fb_init(uint32_t *addr, size_t width, size_t height, size_t pitch, uint16_t bpp);
void fb_put_pixel(size_t x, size_t y, uint32_t color);
void fb_clear(uint32_t color);
framebuffer_t* fb_get(void);

#endif