#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

void terminal_init(void);
void terminal_putchar(char c);
void terminal_write(const char *str);
void terminal_clear(void);
void terminal_set_color(uint32_t fg, uint32_t bg);

#endif