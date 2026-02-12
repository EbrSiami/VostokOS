#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>

// Main kernel printf function
void printk(const char *format, ...);

// Helper for printing without newline
void printk_no_newline(const char *format, ...);

#endif