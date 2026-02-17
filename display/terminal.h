#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

void terminal_init(void);
void terminal_backspace(void);
void terminal_putchar(char c);
void terminal_write(const char *str);
void terminal_clear(void);
void terminal_set_color(uint32_t fg, uint32_t bg);

// New scrolling functions
void terminal_scroll_up(void);    // Scroll view up (see older content)
void terminal_scroll_down(void);  // Scroll view down (see newer content)
void terminal_scroll_to_bottom(void); // Jump to latest content
int terminal_is_at_bottom(void);  // Check if viewing latest content

#endif