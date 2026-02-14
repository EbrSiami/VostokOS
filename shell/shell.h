#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16

// intialize shell
void shell_init(void);

// process a single character from keyboard
void shell_process_char(char c);

// execute a command
void shell_execute(const char *command);

// get shell prompt
const char* shell_get_prompt(void);

#endif