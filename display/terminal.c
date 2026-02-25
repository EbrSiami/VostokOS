#include "terminal.h"
#include "framebuffer.h"
#include "../font/font.h"
#include "../lib/string.h"

#define SCROLLBACK_LINES 20
#define MAX_LINE_LENGTH 160

// Terminal state
static size_t terminal_row = 0;
static size_t terminal_col = 0;
static uint32_t fg_color = 0xFFFFFF;
static uint32_t bg_color = 0x000000;

// Small scrollback buffer: 20 lines × 160 chars = 3.2KB total
static char line_buffer[SCROLLBACK_LINES][MAX_LINE_LENGTH];
static size_t buffer_start = 0;  // Circular buffer start index
static size_t buffer_count = 0;  // Number of lines in buffer
static size_t scroll_offset = 0; // How many lines scrolled up from bottom
static size_t max_cols = 0;
static size_t max_rows = 0;

void terminal_init(void) {
    framebuffer_t *fb = fb_get();
    max_cols = fb->width / FONT_WIDTH;
    max_rows = fb->height / FONT_HEIGHT;
    
    // Limit max_cols to our buffer size
    if (max_cols > MAX_LINE_LENGTH) {
        max_cols = MAX_LINE_LENGTH;
    }
    
    terminal_row = 0;
    terminal_col = 0;
    scroll_offset = 0;
    buffer_count = 0;
    buffer_start = 0;
    
    // Clear buffer
    memset(line_buffer, 0, sizeof(line_buffer));
    
    terminal_clear();
}

static void draw_char(char c, size_t x, size_t y) {
    unsigned char uc = (unsigned char)c;
    
    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t font_row = font_data[uc][row];
        for (size_t col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (font_row & (1 << (7 - col))) ? fg_color : bg_color;
            fb_put_pixel(x + col, y + row, color);
        }
    }
}

// Get line from circular buffer
static char* get_line(size_t index) {
    if (index >= buffer_count) return NULL;
    size_t actual_index = (buffer_start + index) % SCROLLBACK_LINES;
    return line_buffer[actual_index];
}

// Get current line for writing
static char* get_current_line(void) {
    if (buffer_count == 0) return NULL;
    return get_line(buffer_count - 1);
}

// Add a new line to the buffer
static void add_new_line(void) {
    if (buffer_count < SCROLLBACK_LINES) {
        // Still have room
        buffer_count++;
    } else {
        // Buffer full, wrap around
        buffer_start = (buffer_start + 1) % SCROLLBACK_LINES;
    }
    
    // Clear the new line
    char *line = get_current_line();
    if (line) {
        memset(line, 0, MAX_LINE_LENGTH);
    }
}

// Redraw entire screen from buffer
static void redraw_screen(void) {
    fb_clear(bg_color);
    
    if (buffer_count == 0) return;
    
    // Calculate which lines to show
    size_t visible_lines = (buffer_count < max_rows) ? buffer_count : max_rows;
    size_t start_line = (buffer_count > max_rows) ? 
                        buffer_count - max_rows - scroll_offset : 0;
    
    // Don't scroll past the beginning
    if (start_line + visible_lines > buffer_count) {
        start_line = (buffer_count > visible_lines) ? buffer_count - visible_lines : 0;
    }
    
    for (size_t i = 0; i < visible_lines; i++) {
        size_t line_idx = start_line + i;
        if (line_idx >= buffer_count) break;
        
        char *line = get_line(line_idx);
        if (!line) continue;
        
        for (size_t col = 0; col < max_cols && line[col] != '\0'; col++) {
            draw_char(line[col], col * FONT_WIDTH, i * FONT_HEIGHT);
        }
    }
}

// Hardware scroll (copy framebuffer up one line)
static void hardware_scroll(void) {
    framebuffer_t *fb = fb_get();
    
    for (size_t y = 0; y < fb->height - FONT_HEIGHT; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            size_t src_idx = ((y + FONT_HEIGHT) * (fb->pitch / 4)) + x;
            size_t dst_idx = (y * (fb->pitch / 4)) + x;
            fb->backbuffer[dst_idx] = fb->backbuffer[src_idx]; 
        }
    }
    
    // Clear last line
    for (size_t y = fb->height - FONT_HEIGHT; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            fb_put_pixel(x, y, bg_color);
        }
    }
}

void terminal_scroll_up(void) {
    if (buffer_count <= max_rows) return; // Nothing to scroll
    if (scroll_offset >= buffer_count - max_rows) return; // Already at top
    
    scroll_offset++;
    redraw_screen();
}

void terminal_scroll_down(void) {
    if (scroll_offset == 0) return; // Already at bottom
    
    scroll_offset--;
    redraw_screen();
}

void terminal_scroll_to_bottom(void) {
    if (scroll_offset != 0) {
        scroll_offset = 0;
        redraw_screen();
    }
}

int terminal_is_at_bottom(void) {
    return (scroll_offset == 0);
}

void terminal_backspace(void) {
    if (terminal_col > 0) {
        terminal_col--;
        
        // Update buffer
        char *line = get_current_line();
        if (line && terminal_col < MAX_LINE_LENGTH) {
            line[terminal_col] = '\0';
        }
        
        // Update screen if at bottom
        if (scroll_offset == 0) {
            size_t screen_row = (terminal_row < max_rows) ? terminal_row : max_rows - 1;
            draw_char(' ', terminal_col * FONT_WIDTH, screen_row * FONT_HEIGHT);
        }
    }
}

void terminal_putchar(char c) {
    // Initialize buffer on first use
    if (buffer_count == 0) {
        add_new_line();
    }
    
    if (c == '\b') {
        terminal_backspace();
        return;
    }
    
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
        add_new_line();
        
        // Check if we need to scroll the screen
        if (terminal_row >= max_rows) {
            if (scroll_offset == 0) {
                hardware_scroll();
                terminal_row = max_rows - 1;  // Stay on last visible line
            }
            // If scrolled up, terminal_row keeps growing (writing in background)
        }
        return;
    }
    
    if (c == '\r') {
        terminal_col = 0;
        return;
    }
    
    // Store character in buffer
    char *line = get_current_line();
    if (line && terminal_col < max_cols && terminal_col < MAX_LINE_LENGTH) {
        line[terminal_col] = c;
    }
    
    // Draw character if at bottom
    if (scroll_offset == 0) {
        size_t screen_row = (terminal_row < max_rows) ? terminal_row : max_rows - 1;
        draw_char(c, terminal_col * FONT_WIDTH, screen_row * FONT_HEIGHT);
    }
    
    terminal_col++;
    
    // Line wrap
    if (terminal_col >= max_cols) {
        terminal_col = 0;
        terminal_row++;
        add_new_line();
        
        // Check if we need to scroll the screen
        if (terminal_row >= max_rows) {
            if (scroll_offset == 0) {
                hardware_scroll();
                terminal_row = max_rows - 1;  // Stay on last visible line
            }
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
    scroll_offset = 0;
    buffer_count = 0;
    buffer_start = 0;
    memset(line_buffer, 0, sizeof(line_buffer));
    add_new_line();
}

void terminal_set_color(uint32_t fg, uint32_t bg) {
    fg_color = fg;
    bg_color = bg;
}

void terminal_put_repeated(char c, size_t count) {
    for (size_t i = 0; i < count; i++) {
        terminal_putchar(c);
    }
}