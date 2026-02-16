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

// Forward declarations of command handlers
static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_info(int argc, char **argv);
static void cmd_reboot(int argc, char **argv);
static void cmd_uptime(int argc, char **argv);
static void cmd_meminfo(int argc, char **argv);

// Command structure
typedef struct {
    const char *name;
    const char *description;
    void (*handler)(int argc, char **argv);
} shell_command_t;

// Built-in commands
static shell_command_t commands[] = {
    {"help",    "Display available commands",              cmd_help},
    {"clear",   "Clear the screen",                        cmd_clear},
    {"echo",    "Print arguments to screen",               cmd_echo},
    {"info",    "Display system information",              cmd_info},
    {"reboot",  "Reboot the system",                       cmd_reboot},
    {"uptime",  "Display system uptime",                   cmd_uptime},
    {"meminfo", "Display memory information",              cmd_meminfo},
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

// Command handlers
static void cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("\nAvailable commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printk("  %-10s - %s\n", commands[i].name, commands[i].description);
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
    
    printk("\n=== VostokOS System Information ===\n");
    printk("OS Name:     VostokOS\n");
    printk("Version:     0.1.0-dev\n");
    printk("Architecture: x86_64\n");
    printk("Bootloader:  Limine\n");
    printk("\nFeatures:\n");
    printk("  [x] Framebuffer graphics\n");
    printk("  [x] Interrupt handling (GDT/IDT)\n");
    printk("  [x] PS/2 Keyboard driver\n");
    printk("  [x] PIC remapping\n");
    printk("  [x] Basic shell\n");
    printk("FUCK ISRAEL BTW");
    printk("\n");
}

static void cmd_uptime(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    uint64_t uptime = timer_get_uptime();
    uint64_t hours = uptime / 3600;
    uint64_t minutes = (uptime % 3600) / 60;
    uint64_t seconds = uptime % 60;
    
    printk("Uptime: %llu hours, %llu minutes, %llu seconds\n", 
           hours, minutes, seconds);
    printk("Total ticks: %llu\n", timer_get_ticks());
}

static void cmd_reboot(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("Rebooting...\n");
    for (volatile int i = 0; i < 10000000; i++);
    
    __asm__ volatile ("cli");
    
    // Method 1: Keyboard controller (most compatible)
    uint8_t temp;
    do {
        __asm__ volatile ("inb $0x64, %0" : "=a"(temp));
        if (temp & 0x01) {
            __asm__ volatile ("inb $0x60, %0" : "=a"(temp));
        }
    } while (temp & 0x02);
    __asm__ volatile ("outb %0, $0x64" : : "a"((uint8_t)0xFE));
    
    // Method 2: Triple fault (if keyboard fails)
    uint16_t *invalid_idt = (uint16_t*)0x0;
    invalid_idt[0] = 0;
    __asm__ volatile ("lidt (%0)" : : "r"(invalid_idt));
    __asm__ volatile ("int $0x00");
    
    // Method 3: Just halt if all else fails
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void shell_init(void) {
    memset(command_buffer, 0, SHELL_BUFFER_SIZE);
    buffer_pos = 0;
    
    printk("\n=== Welcome to VostokOS ===\n");
    printk("Type 'help' for available commands\n\n");
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
            shell_execute(command_buffer);
        }
        
        // Reset buffer
        buffer_pos = 0;
        memset(command_buffer, 0, SHELL_BUFFER_SIZE);
        
        // Print new prompt
        printk("%s", prompt);
        
    } else if (c == '\b') {
        // Backspace
        if (buffer_pos > 0) {
            buffer_pos--;
            command_buffer[buffer_pos] = '\0';
            
            // Just call backspace once - terminal handles it now
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
    
    // Make a copy to parse (parsing modifies the string)
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
    printk("Unknown command: %s\n", argv[0]);
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
    
    printk("\n=== Memory Information ===\n");
    printk("Total Memory:  %llu MB (%llu KB)\n", 
           total_mem / (1024 * 1024), total_mem / 1024);
    printk("Usable Memory: %llu MB (%llu KB)\n", 
           usable_mem / (1024 * 1024), usable_mem / 1024);
    printk("Reserved:      %llu MB\n\n", 
           (total_mem - usable_mem) / (1024 * 1024));
    
    printk("Memory Map (%llu entries):\n", memmap->entry_count);
    
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
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
                type_str = "ACPI Reclaimable";
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
        
        printk("  [%llu] 0x%llx - 0x%llx (%llu KB) - %s\n",
               i,
               entry->base,
               entry->base + entry->length,
               entry->length / 1024,
               type_str);
    }
    
    printk("\n");
}