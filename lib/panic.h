#ifndef PANIC_H
#define PANIC_H

// Macro to automatically pass file and line
#define panic(msg) panic_impl(msg, __FILE__, __LINE__)

void panic_impl(const char *message, const char *file, int line);

#endif