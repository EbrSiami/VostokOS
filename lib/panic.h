#ifndef PANIC_H
#define PANIC_H

// prevent Forward declaration
struct interrupt_registers;

void panic_impl(const char *message, const char *file, int line, struct interrupt_registers *regs);

// Macro to automatically pass file and line
#define panic(msg) panic_impl(msg, __FILE__, __LINE__, 0)

// Macro to CPU Exceptions
#define panic_exception(msg, regs) panic_impl(msg, __FILE__, __LINE__, regs)

#endif