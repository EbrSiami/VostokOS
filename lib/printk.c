#include "printk.h"
#include "string.h"
#include "../display/terminal.h"

static void print_number(uint64_t value, int base, int is_signed, int uppercase) {
    char buffer[65]; // Max size for 64-bit binary + null terminator
    
    if (is_signed && base == 10) {
        itoa((int64_t)value, buffer, base);
    } else if (base == 16) {
        itoa_hex(value, buffer, uppercase);
    } else {
        utoa(value, buffer, base);
    }
    
    terminal_write(buffer);
}

void printk(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    for (size_t i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }
        
        // Handle format specifiers
        i++;
        switch (format[i]) {
            case 'c': // Character
                terminal_putchar((char)va_arg(args, int));
                break;
                
            case 's': // String
                terminal_write(va_arg(args, const char *));
                break;
                
            case 'd': // Signed decimal
            case 'i':
                print_number(va_arg(args, int), 10, 1, 0);
                break;
                
            case 'u': // Unsigned decimal
                print_number(va_arg(args, unsigned int), 10, 0, 0);
                break;
                
            case 'x': // Hex lowercase
                print_number(va_arg(args, unsigned int), 16, 0, 0);
                break;
                
            case 'X': // Hex uppercase
                print_number(va_arg(args, unsigned int), 16, 0, 1);
                break;
                
            case 'p': // Pointer
                terminal_write("0x");
                print_number((uint64_t)va_arg(args, void *), 16, 0, 0);
                break;
                
            case 'l': // Long modifier
                i++;
                if (format[i] == 'l') { // long long
                    i++;
                    switch (format[i]) {
                        case 'd':
                        case 'i':
                            print_number(va_arg(args, long long), 10, 1, 0);
                            break;
                        case 'u':
                            print_number(va_arg(args, unsigned long long), 10, 0, 0);
                            break;
                        case 'x':
                            print_number(va_arg(args, unsigned long long), 16, 0, 0);
                            break;
                        case 'X':
                            print_number(va_arg(args, unsigned long long), 16, 0, 1);
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
                            print_number(va_arg(args, long), 10, 1, 0);
                            break;
                        case 'u':
                            print_number(va_arg(args, unsigned long), 10, 0, 0);
                            break;
                        case 'x':
                            print_number(va_arg(args, unsigned long), 16, 0, 0);
                            break;
                        case 'X':
                            print_number(va_arg(args, unsigned long), 16, 0, 1);
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
                terminal_putchar(format[i]);
                break;
        }
    }
    
    va_end(args);
}