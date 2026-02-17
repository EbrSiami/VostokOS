#include "shell.h"
#include "../limine.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "../lib/memory.h"
#include "../display/terminal.h"
#include "../drivers/timer.h"

// Command buffer
static char command_buffer[SHELL_BUFFER_SIZE];
static size_t buffer_pos = 0;

// Shell prompt
static const char *prompt = "vostok> ";

// Command history (simple version)
#define HISTORY_SIZE 10
static char history[HISTORY_SIZE][SHELL_BUFFER_SIZE];
static int history_count = 0;
static int history_index = -1;

// Forward declarations of command handlers
static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_info(int argc, char **argv);
static void cmd_reboot(int argc, char **argv);
static void cmd_uptime(int argc, char **argv);
static void cmd_meminfo(int argc, char **argv);
static void cmd_sleep(int argc, char **argv);
static void cmd_benchmark(int argc, char **argv);
static void cmd_spinner(int argc, char **argv);
static void cmd_matrix(int argc, char **argv);
static void cmd_history(int argc, char **argv);

// Command structure
typedef struct {
    const char *name;
    const char *description;
    void (*handler)(int argc, char **argv);
} shell_command_t;

// Built-in commands
static shell_command_t commands[] = {
    {"help",      "Display available commands",          cmd_help},
    {"clear",     "Clear the screen",                    cmd_clear},
    {"echo",      "Print arguments to screen",           cmd_echo},
    {"info",      "Display system information",          cmd_info},
    {"reboot",    "Reboot the system",                   cmd_reboot},
    {"uptime",    "Display system uptime",               cmd_uptime},
    {"meminfo",   "Display memory information",          cmd_meminfo},
    {"sleep",     "Sleep for N seconds",                 cmd_sleep},
    {"benchmark", "Run CPU benchmark",                   cmd_benchmark},
    {"spinner",   "Show loading spinner",                cmd_spinner},
    {"matrix",    "Matrix rain effect",                  cmd_matrix},
    {"history",   "Show command history",                cmd_history},
    {NULL, NULL, NULL}  // Sentinel
};

// Parse command into arguments
static int parse_command(char *cmd, char **argv, int max_args) {
    int argc = 0;
    char *ptr = cmd;
    bool in_word = false;
    
    while (*ptr && argc < max_args) {
        if (*ptr == ' ' || *ptr == '\t') {
            *ptr = '\0';
            in_word = false;
        } else if (!in_word) {
            argv[argc++] = ptr;
            in_word = true;
        }
        ptr++;
    }
    
    return argc;
}

static void draw_shell_box(const char* title) {
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

// Add command to history
static void add_to_history(const char *cmd) {
    if (strlen(cmd) == 0) return;
    
    // Shift history if full
    if (history_count >= HISTORY_SIZE) {
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strcpy(history[i-1], history[i]);
        }
        history_count = HISTORY_SIZE - 1;
    }
    
    // Add new command
    strncpy(history[history_count], cmd, SHELL_BUFFER_SIZE - 1);
    history[history_count][SHELL_BUFFER_SIZE - 1] = '\0';
    history_count++;
}

// Command handlers
static void cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("\n");
    draw_shell_box("VostokOS Command Reference");
    
    printk("\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printk("  %-12s - %s\n", commands[i].name, commands[i].description);
    }
    printk("\n");
}

static void cmd_clear(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    terminal_clear();
}

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printk("%s", argv[i]);
        if (i < argc - 1) {
            printk(" ");
        }
    }
    printk("\n");
}

static void cmd_info(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("\n");
    draw_shell_box("VostokOS System Information");
    printk("\n");

    printk("  OS Name:       VostokOS\n");
    printk("  Version:       0.1.3-dev\n");
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

static void cmd_uptime(int argc, char **argv) {
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

static void cmd_sleep(int argc, char **argv) {
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

static void cmd_benchmark(int argc, char **argv) {
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

static void cmd_spinner(int argc, char **argv) {
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

static void cmd_matrix(int argc, char **argv) {
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

static void cmd_history(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    if (history_count == 0) {
        printk("No command history.\n");
        return;
    }
    
    printk("\nCommand History:\n");
    for (int i = 0; i < history_count; i++) {
        printk("  %d  %s\n", i + 1, history[i]);
    }
    printk("\n");
}

static void cmd_reboot(int argc, char **argv) {
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

void shell_init(void) {
    memset(command_buffer, 0, SHELL_BUFFER_SIZE);
    buffer_pos = 0;
    history_count = 0;
    history_index = -1;
    
    // Cool startup banner
    terminal_set_color(0x00FFFF, 0x000000);  // Cyan
    printk("\n");
    draw_shell_box("Welcome to VostokOS v0.1.3");
    terminal_set_color(0xFFFFFF, 0x000000);  // White
    
    printk("\n");
    printk("Type 'help' for available commands\n");
    printk("Type 'info' for system information\n");
    printk("\n");
    printk("%s", prompt);
}

const char* shell_get_prompt(void) {
    return prompt;
}

void shell_process_char(char c) {
    if (c == '\n') {
        // Execute command
        printk("\n");
        
        if (buffer_pos > 0) {
            command_buffer[buffer_pos] = '\0';
            add_to_history(command_buffer);
            shell_execute(command_buffer);
        }
        
        // Reset buffer
        buffer_pos = 0;
        memset(command_buffer, 0, SHELL_BUFFER_SIZE);
        history_index = -1;
        
        // Print new prompt
        printk("%s", prompt);
        
    } else if (c == '\b') {
        // Backspace
        if (buffer_pos > 0) {
            buffer_pos--;
            command_buffer[buffer_pos] = '\0';
            terminal_putchar('\b');
        }
        
    } else if (c >= 32 && c <= 126) {
        // Printable character
        if (buffer_pos < SHELL_BUFFER_SIZE - 1) {
            command_buffer[buffer_pos++] = c;
            terminal_putchar(c);
        }
    }
}


void shell_execute(const char *command) {
    // Skip leading whitespace
    while (*command == ' ' || *command == '\t') {
        command++;
    }
    
    // Empty command
    if (*command == '\0') {
        return;
    }
    
    // Make a copy to parse
    char cmd_copy[SHELL_BUFFER_SIZE];
    strncpy(cmd_copy, command, SHELL_BUFFER_SIZE - 1);
    cmd_copy[SHELL_BUFFER_SIZE - 1] = '\0';
    
    // Parse into arguments
    char *argv[SHELL_MAX_ARGS];
    int argc = parse_command(cmd_copy, argv, SHELL_MAX_ARGS);
    
    if (argc == 0) {
        return;
    }
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }
    
    // Command not found
    terminal_set_color(0xFF6666, 0x000000);  // Light red
    printk("Error: ");
    terminal_set_color(0xFFFFFF, 0x000000);  // White
    printk("Unknown command '%s'\n", argv[0]);
    printk("Type 'help' for available commands.\n");
}

static void cmd_meminfo(int argc, char **argv) {
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
    printk("  %s\n", "────────────────────────────────────────────────────────────────");
    
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