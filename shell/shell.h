#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16

typedef struct {
    const char *name;
    const char *description;
    void (*handler)(int argc, char **argv);
} shell_command_t;

// shell engine main functions
void shell_init(void);
void shell_process_char(char c);
const char* shell_get_prompt(void);
void shell_execute(const char *command);

// helping functions
void draw_shell_box(const char* title);

// list of all commands
void cmd_help(int argc, char **argv);
void cmd_clear(int argc, char **argv);
void cmd_echo(int argc, char **argv);
void cmd_info(int argc, char **argv);
void cmd_shutdown(int argc, char **argv);
void cmd_reboot(int argc, char **argv);
void cmd_uptime(int argc, char **argv);
void cmd_meminfo(int argc, char **argv);
void cmd_sleep(int argc, char **argv);
void cmd_benchmark(int argc, char **argv);
void cmd_spinner(int argc, char **argv);
void cmd_matrix(int argc, char **argv);
void cmd_history(int argc, char **argv);

int shell_get_history_count(void);
const char* shell_get_history_item(int index);
const shell_command_t* shell_get_commands_list(void);

#endif