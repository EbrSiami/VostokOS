#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t *address;
    uint32_t *backbuffer;
    size_t width;
    size_t height;
    size_t pitch;
    uint16_t bpp;
} framebuffer_t;

void fb_init(uint32_t *addr, size_t width, size_t height, size_t pitch, uint16_t bpp);
void fb_enable_double_buffering(void);

void fb_put_pixel(size_t x, size_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_swap(void);

// GUI Primitives
void fb_fill_rect(size_t x, size_t y, size_t w, size_t h, uint32_t color);
void fb_draw_rect(size_t x, size_t y, size_t w, size_t h, uint32_t color);
void fb_draw_line(size_t x0, size_t y0, size_t x1, size_t y1, uint32_t color);
void fb_blit(size_t x, size_t y, size_t w, size_t h, uint32_t* bitmap);

uint32_t fb_get_pixel(size_t x, size_t y);

framebuffer_t* fb_get(void);

#endif