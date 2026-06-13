#include "framebuffer.h"
#include "../lib/string.h"
#include "../mm/heap.h"
#include "../lib/printk.h"

static framebuffer_t fb;

void fb_init(uint32_t *addr, size_t width, size_t height, size_t pitch, uint16_t bpp) {
    fb.address = addr;
    fb.backbuffer = addr; // Early boot: Write directly to VRAM
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
        // Copy what's currently on the screen into our new backbuffer
        memcpy(fb.backbuffer, fb.address, buffer_size);
        printk("[FB] Double buffering enabled successfully! (%llu MB allocated)\n", buffer_size / (1024 * 1024));
    }
}

void fb_put_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= fb.width || y >= fb.height) return;
    
    size_t pixel_index = (y * (fb.pitch / 4)) + x;
    fb.backbuffer[pixel_index] = color;
}

void fb_clear(uint32_t color) {
    if (color == 0) {
        // We can safely memset the whole pitch * height for black
        memset(fb.backbuffer, 0, fb.height * fb.pitch);
        return;
    }

    // Pitch-aware clearing for colors
    size_t stride = fb.pitch / 4;
    for (size_t y = 0; y < fb.height; y++) {
        for (size_t x = 0; x < fb.width; x++) {
            fb.backbuffer[(y * stride) + x] = color;
        }
    }
}

void fb_swap(void) {
    if (fb.backbuffer == fb.address) return;
    
    uint64_t* src = (uint64_t*)fb.backbuffer;
    uint64_t* dst = (uint64_t*)fb.address;
    size_t count = (fb.height * fb.pitch) / 8;
    
    for (size_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

// GUI Primitives
void fb_fill_rect(size_t x, size_t y, size_t w, size_t h, uint32_t color) {
    if (x >= fb.width || y >= fb.height) return;
    
    // Hardware clipping (overflow)
    if (x + w > fb.width) w = fb.width - x;
    if (y + h > fb.height) h = fb.height - y;

    size_t stride = fb.pitch / 4;
    
    for (size_t py = y; py < y + h; py++) {
        uint32_t* row_ptr = &fb.backbuffer[(py * stride) + x];
        for (size_t px = 0; px < w; px++) {
            row_ptr[px] = color;
        }
    }
}

void fb_draw_rect(size_t x, size_t y, size_t w, size_t h, uint32_t color) {
    fb_fill_rect(x, y, w, 1, color);           // Top
    fb_fill_rect(x, y + h - 1, w, 1, color);   // Bottom
    fb_fill_rect(x, y, 1, h, color);           // Left
    fb_fill_rect(x + w - 1, y, 1, h, color);   // Right
}

void fb_draw_line(size_t x0, size_t y0, size_t x1, size_t y1, uint32_t color) {
    // Bresenham's line algorithm
    int dx = (int)x1 - (int)x0;
    int dy = (int)y1 - (int)y0;
    int sx = dx > 0 ? 1 : -1;
    int sy = dy > 0 ? 1 : -1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = dx - dy;
    
    while (1) {
        fb_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void fb_blit(size_t x, size_t y, size_t w, size_t h, uint32_t* bitmap) {
    for (size_t py = 0; py < h; py++) {
        for (size_t px = 0; px < w; px++) {
            uint32_t color = bitmap[py * w + px];
            
            uint8_t alpha = (color >> 24) & 0xFF;
            
            if (alpha > 0) {
                fb_put_pixel(x + px, y + py, color);
            }
        }
    }
}

uint32_t fb_get_pixel(size_t x, size_t y) {
    if (x >= fb.width || y >= fb.height) return 0;
    size_t pixel_index = (y * (fb.pitch / 4)) + x;
    return fb.backbuffer[pixel_index];
}

framebuffer_t* fb_get(void) {
    return &fb;
}