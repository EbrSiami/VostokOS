#include "shell.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "../display/terminal.h"

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

static void cmd_reboot(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printk("Rebooting...\n");
    
    // Give time for message to display
    for (volatile int i = 0; i < 10000000; i++);
    
    // Use keyboard controller to reboot (classic method)
    uint8_t temp;
    __asm__ volatile ("cli");  // Disable interrupts
    
    // Clear keyboard controller
    do {
        __asm__ volatile ("inb $0x64, %0" : "=a"(temp));
        if (temp & 0x01) {
            __asm__ volatile ("inb $0x60, %0" : "=a"(temp));
        }
    } while (temp & 0x02);
    
    // Send reboot command
    __asm__ volatile ("outb %0, $0x64" : : "a"((uint8_t)0xFE));
    
    // If that didn't work, triple fault
    for (;;) {
        __asm__ volatile ("cli; hlt");
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