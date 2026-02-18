#include "shell.h"
#include "../lib/printk.h"
#include "../lib/string.h"
#include "../display/terminal.h"

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

// Add command to history
void add_to_history(const char *cmd) {
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

// Parse command into arguments
int parse_command(char *cmd, char **argv, int max_args) {
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

const char* shell_get_prompt(void) {
    return prompt;
}

int shell_get_history_count(void) {
    return history_count;
}

const char* shell_get_history_item(int index) {
    if (index < 0 || index >= history_count) {
        return NULL;
    }
    return history[index];
}

const shell_command_t* shell_get_commands_list(void) {
    return commands;
}