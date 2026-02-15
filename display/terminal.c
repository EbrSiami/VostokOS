#include "terminal.h"
#include "framebuffer.h"
#include "../font/font.h"

static size_t terminal_row = 0;
static size_t terminal_col = 0;
static uint32_t fg_color = 0xFFFFFF; // White
static uint32_t bg_color = 0x000000; // Black

void terminal_init(void) {
    terminal_row = 0;
    terminal_col = 0;
    terminal_clear();
}

static void draw_char(char c, size_t x, size_t y) {
    
    unsigned char uc = (unsigned char)c;

    if (uc >= 128) {
        uc = '?';
    }
    
    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t font_row = font_data[(uint8_t)c][row];
        for (size_t col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (font_row & (1 << (7 - col))) ? fg_color : bg_color;
            fb_put_pixel(x + col, y + row, color);
        }
    }
}

static void scroll(void) {
    // For now, just wrap to top.
    terminal_row = 0;
    terminal_col = 0;
}

void terminal_backspace(void) {
    if (terminal_col > 0) {
        terminal_col--;
        // Redraw current position with space (erase character)
        draw_char(' ', terminal_col * FONT_WIDTH, terminal_row * FONT_HEIGHT);
    } else if (terminal_row > 0) {
        // Move to end of previous line
        framebuffer_t *fb = fb_get();
        terminal_row--;
        terminal_col = (fb->width / FONT_WIDTH) - 1;
    }
}

void terminal_putchar(char c) {
    framebuffer_t *fb = fb_get();
    size_t max_cols = fb->width / FONT_WIDTH;
    size_t max_rows = fb->height / FONT_HEIGHT;
    
    if (c == '\b') {
        terminal_backspace();  // Use the new function
        return;
    }
    
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= max_rows) {
            scroll();
        }
        return;
    }
    
    if (c == '\r') {
        terminal_col = 0;
        return;
    }
    
    draw_char(c, terminal_col * FONT_WIDTH, terminal_row * FONT_HEIGHT);
    terminal_col++;
    
    if (terminal_col >= max_cols) {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= max_rows) {
            scroll();
        }
    }
}

void terminal_write(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_clear(void) {
    fb_clear(bg_color);
    terminal_row = 0;
    terminal_col = 0;
}

void terminal_set_color(uint32_t fg, uint32_t bg) {
    fg_color = fg;
    bg_color = bg;
}