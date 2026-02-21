#include "shell.h"
#include "../lib/printk.h"
#include "../lib/string.h"
#include "../lib/memory.h"
#include "../display/terminal.h"
#include "../drivers/timer.h"
#include "../limine.h"

void draw_shell_box(const char* title) {
    const int total_width = 50; // A fixed width for all boxes
    int title_len = strlen(title);
    int padding = (total_width - title_len) / 2;

    // Top line
    printk("\xC9");
    terminal_put_repeated('\xCD', total_width);
    printk("\xBB\n");

    // center line
    printk("\xBA");
    terminal_put_repeated(' ', padding);
    printk("%s", title);
    // Calculating the remaining space for precise right edge alignment
    int remaining = total_width - title_len - padding;
    terminal_put_repeated(' ', remaining);
    printk("\xBA\n");

    // bottom line
    printk("\xC8");
    terminal_put_repeated('\xCD', total_width);
    printk("\xBC\n");
}

// Command handlers
void cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("\n");
    draw_shell_box("VostokOS Command Reference");
    printk("\n");

    const shell_command_t* cmds = shell_get_commands_list();

    for (int i = 0; cmds[i].name != NULL; i++) {
        printk("  %-12s - %s\n", cmds[i].name, cmds[i].description);
    }
    printk("\n");
}

void cmd_clear(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    terminal_clear();
}

void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printk("%s", argv[i]);
        if (i < argc - 1) {
            printk(" ");
        }
    }
    printk("\n");
}

void cmd_info(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("\n");
    draw_shell_box("VostokOS System Information");
    printk("\n");

    printk("  OS Name:       VostokOS\n");
    printk("  Version:       0.1.8-dev\n");
    printk("  Architecture:  x86_64\n");
    printk("  Bootloader:    Limine\n");
    printk("  Author:        Ebrahim Siami \n");
    
    printk("\n  Features:\n");
    printk("    \xFB Framebuffer graphics\n");
    printk("    \xFB Interrupt handling (GDT/IDT)\n");
    printk("    \xFB PS/2 Keyboard driver\n");
    printk("    \xFB PIT Timer (%u Hz)\n", timer_get_frequency());
    printk("    \xFB Terminal with scrollback\n");
    printk("    \xFB Advanced shell\n");
    
    terminal_set_color(0x00FF00, 0x000000);
    printk("\n  FREE PALESTINE! \n");
    terminal_set_color(0xFFFFFF, 0x000000);
    
    printk("\n");
}

void cmd_uptime(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    uint64_t uptime_sec = timer_get_uptime();
    uint64_t uptime_ms = timer_get_uptime_ms();
    
    uint64_t days = uptime_sec / 86400;
    uint64_t hours = (uptime_sec % 86400) / 3600;
    uint64_t minutes = (uptime_sec % 3600) / 60;
    uint64_t seconds = uptime_sec % 60;
    uint64_t milliseconds = uptime_ms % 1000;
    
    printk("System uptime: ");
    
    if (days > 0) {
        printk("%llu day%s, ", days, days == 1 ? "" : "s");
    }
    
    printk("%02llu:%02llu:%02llu.%03llu\n", hours, minutes, seconds, milliseconds);
    printk("Total ticks: %llu\n", timer_get_ticks());
}

void cmd_sleep(int argc, char **argv) {
    if (argc < 2) {
        printk("Usage: sleep <seconds>\n");
        return;
    }
    
    // Simple atoi implementation
    int seconds = 0;
    const char *str = argv[1];
    while (*str >= '0' && *str <= '9') {
        seconds = seconds * 10 + (*str - '0');
        str++;
    }
    
    if (seconds <= 0 || seconds > 60) {
        printk("Error: Sleep time must be between 1-60 seconds\n");
        return;
    }
    
    printk("Sleeping for %d second%s", seconds, seconds == 1 ? "" : "s");
    
    // Show dots while sleeping
    for (int i = 0; i < seconds; i++) {
        timer_sleep(1);
        printk(".");
    }
    
    printk(" Done!\n");
}

void cmd_benchmark(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    draw_shell_box("CPU Benchmark Test");
    
    printk("Running arithmetic test...\n");
    
    uint64_t start_ms = timer_get_uptime_ms();
    
    volatile uint64_t sum = 0;
    for (uint64_t i = 0; i < 10000000; i++) {
        sum += i;
    }
    
    uint64_t end_ms = timer_get_uptime_ms();
    uint64_t elapsed = end_ms - start_ms;
    
    printk("  Result: %llu\n", sum);
    printk("  Time:   %llu ms\n", elapsed);
    
    if (elapsed > 0) {
        uint64_t ops_per_sec = 10000000000ULL / elapsed;
        printk("  Speed:  ~%llu million ops/sec\n", ops_per_sec / 1000000);
    }
    
    printk("\n");
}

void cmd_spinner(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    const char spinner[] = {'|', '/', '-', '\\'};
    
    printk("Loading");
    
    for (int i = 0; i < 40; i++) {
        printk(" %c", spinner[i % 4]);
        timer_sleep_ms(100);
        printk("\b\b");  // Go back 2 chars
    }
    
    printk(" ✓ Done!\n");
}

void cmd_matrix(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    terminal_set_color(0x00FF00, 0x000000);  // Green text
    printk("Matrix mode... (5 seconds)\n\n");
    
    /* ASCII only - avoid multi-byte UTF-8 / encoding issues */
    const char chars[] = "01";
    uint64_t end_time = timer_get_uptime_ms() + 5000;
    
    while (timer_get_uptime_ms() < end_time) {
        /* Pick char by tick (pseudo-random) */
        char c = chars[timer_get_ticks() % (sizeof(chars) - 1)];
        terminal_putchar(c);
        
        timer_sleep_ms(20);
        
        // Random newlines
        if ((timer_get_ticks() % 40) == 0) {
            terminal_putchar('\n');
        }
    }
    
    terminal_set_color(0xFFFFFF, 0x000000);  // Back to white
    printk("\n\n[Matrix ended]\n");
}

void cmd_history(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    int count = shell_get_history_count();

    if (count == 0) {
        printk("No command history.\n");
        return;
    }
    
    printk("\nCommand History:\n");
    for (int i = 0; i < count; i++) {
        const char* item = shell_get_history_item(i);
        if (item) {
            printk("  %d  %s\n", i + 1, item);
        }
    }
    printk("\n");
}

void cmd_reboot(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    draw_shell_box("System Reboot");
    
    printk("Shutting down VostokOS");
    
    for (int i = 0; i < 3; i++) {
        timer_sleep(1);
        printk(".");
    }
    
    printk("\n\nRebooting now...\n\n");
    timer_sleep(1);
    
    __asm__ volatile ("cli");
    
    // Method 1: Keyboard controller
    uint8_t temp;
    do {
        __asm__ volatile ("inb $0x64, %0" : "=a"(temp));
        if (temp & 0x01) {
            __asm__ volatile ("inb $0x60, %0" : "=a"(temp));
        }
    } while (temp & 0x02);
    __asm__ volatile ("outb %0, $0x64" : : "a"((uint8_t)0xFE));
    
    // Method 2: Triple fault
    uint16_t *invalid_idt = (uint16_t*)0x0;
    invalid_idt[0] = 0;
    __asm__ volatile ("lidt (%0)" : : "r"(invalid_idt));
    __asm__ volatile ("int $0x00");
    
    // Halt if all else fails
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void cmd_meminfo(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    struct limine_memmap_response *memmap = get_memory_map();
    
    if (memmap == NULL) {
        printk("Error: Memory map not available\n");
        return;
    }
    
    uint64_t total_mem = get_total_memory();
    uint64_t usable_mem = get_usable_memory();
    
    draw_shell_box("Memory Information");
    
    printk("  Total Memory:    %llu MB (%llu KB)\n", 
           total_mem / (1024 * 1024), total_mem / 1024);
    printk("  Usable Memory:   %llu MB (%llu KB)\n", 
           usable_mem / (1024 * 1024), usable_mem / 1024);
    printk("  Reserved:        %llu MB\n", 
           (total_mem - usable_mem) / (1024 * 1024));
    printk("  Memory Entries:  %llu\n\n", memmap->entry_count);
    
    printk("Memory Map:\n");
    printk("  %-4s %-18s %-18s %-10s %s\n", 
           "#", "Base", "End", "Size", "Type");
    printk("  ");
    terminal_put_repeated('\xC4', 60);  // CP437 single horizontal
    printk("\n");
    for (uint64_t i = 0; i < memmap->entry_count && i < 15; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        
        const char *type_str;
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                type_str = "Usable";
                break;
            case LIMINE_MEMMAP_RESERVED:
                type_str = "Reserved";
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                type_str = "ACPI Reclaim";
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                type_str = "ACPI NVS";
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                type_str = "Bad Memory";
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                type_str = "Bootloader";
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                type_str = "Kernel";
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                type_str = "Framebuffer";
                break;
            default:
                type_str = "Unknown";
                break;
        }
        
        printk("  %-4llu 0x%-16llx 0x%-16llx %-8llu KB %s\n",
               i,
               entry->base,
               entry->base + entry->length,
               entry->length / 1024,
               type_str);
    }
    
    if (memmap->entry_count > 15) {
        printk("  ... (%llu more entries)\n", memmap->entry_count - 15);
    }
    
    printk("\n");
}