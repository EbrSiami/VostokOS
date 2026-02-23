#include "printk.h"
#include "string.h"
#include "../display/terminal.h"
#include "spinlock.h"

static spinlock_t printk_lock;
static bool printk_lock_init = false;

static void print_number(uint64_t value, int base, int is_signed, int uppercase, int width, int left_justify) {
    char buffer[65];
    
    if (is_signed && base == 10) {
        itoa((int64_t)value, buffer, base);
    } else if (base == 16) {
        itoa_hex(value, buffer, uppercase);
    } else {
        utoa(value, buffer, base);
    }
    
    size_t len = strlen(buffer);
    int padding = 0;
    
    if (width > (int)len) {
        padding = width - len;
    }
    
    if (!left_justify) {
        while (padding > 0) {
            terminal_putchar(' ');
            padding--;
        }
    }
    
    terminal_write(buffer);
    
    if (left_justify) {
        while (padding > 0) {
            terminal_putchar(' ');
            padding--;
        }
    }
}

void printk(const char *format, ...) {

    if (!printk_lock_init) {
        spinlock_init(&printk_lock);
        printk_lock_init = true;
    }
    spinlock_acquire(&printk_lock);

    va_list args;
    va_start(args, format);
    
    for (size_t i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }
        
        // Handle format specifiers
        i++;

        int left_justify = 0;
        if (format[i] == '-') {
            left_justify = 1;
            i++;
        }

        int width = 0;
        while (format[i] >= '0' && format[i] <= '9') {
            width = width * 10 + (format[i] - '0');
            i++;
        }

        switch (format[i]) {
            case 'c': {
                char c = (char)va_arg(args, int);
                if (!left_justify && width > 1) {
                    for(int k=0; k < width-1; k++) terminal_putchar(' ');
                }
                terminal_putchar(c);
                if (left_justify && width > 1) {
                    for(int k=0; k < width-1; k++) terminal_putchar(' ');
                }
                break;
            }
                
            case 's': {
                const char *s = va_arg(args, const char *);
                size_t len = strlen(s);
                int padding = 0;
                if (width > (int)len) padding = width - len;

                if (!left_justify) {
                    while (padding-- > 0) terminal_putchar(' ');
                }
                terminal_write(s);
                if (left_justify) {
                    while (padding-- > 0) terminal_putchar(' ');
                }
                break;
            }
                
            case 'd': // Signed decimal
            case 'i':
                print_number(va_arg(args, int), 10, 1, 0, width, left_justify);
                break;
                
            case 'u': // Unsigned decimal
                print_number(va_arg(args, unsigned int), 10, 0, 0, width, left_justify);
                break;
                
            case 'x': // Hex lowercase
                print_number(va_arg(args, unsigned int), 16, 0, 0, width, left_justify);
                break;
                
            case 'X': // Hex uppercase
                print_number(va_arg(args, unsigned int), 16, 0, 1, width, left_justify);
                break;
                
            case 'p': // Pointer
                terminal_write("0x");
                print_number((uint64_t)va_arg(args, void *), 16, 0, 0, width, left_justify);
                break;
                
            case 'l': // Long modifier
                i++;
                if (format[i] == 'l') { // long long
                    i++;
                    switch (format[i]) {
                        case 'd':
                        case 'i':
                            print_number(va_arg(args, long long), 10, 1, 0, width, left_justify);
                            break;
                        case 'u':
                            print_number(va_arg(args, unsigned long long), 10, 0, 0, width, left_justify);
                            break;
                        case 'x':
                            print_number(va_arg(args, unsigned long long), 16, 0, 0, width, left_justify);
                            break;
                        case 'X':
                            print_number(va_arg(args, unsigned long long), 16, 0, 1, width, left_justify);
                            break;
                        default:
                            terminal_putchar('%');
                            terminal_putchar('l');
                            terminal_putchar('l');
                            terminal_putchar(format[i]);
                            break;
                    }
                } else { // just long
                    switch (format[i]) {
                        case 'd':
                        case 'i':
                            print_number(va_arg(args, long), 10, 1, 0, width, left_justify);
                            break;
                        case 'u':
                            print_number(va_arg(args, unsigned long), 10, 0, 0, width, left_justify);
                            break;
                        case 'x':
                            print_number(va_arg(args, unsigned long), 16, 0, 0, width, left_justify);
                            break;
                        case 'X':
                            print_number(va_arg(args, unsigned long), 16, 0, 1, width, left_justify);
                            break;
                        default:
                            terminal_putchar('%');
                            terminal_putchar('l');
                            terminal_putchar(format[i]);
                            break;
                    }
                }
                break;
                
            case '%': // Literal %
                terminal_putchar('%');
                break;
                
            default: // Unknown specifier
                terminal_putchar('%');
                if (left_justify) terminal_putchar('-');
                terminal_putchar(format[i]);
                break;
        }
    }
    
    va_end(args);

    spinlock_release(&printk_lock);
}

void printk_force_unlock(void) {
    if (printk_lock_init) {
        printk_lock.locked = 0;
    }
}