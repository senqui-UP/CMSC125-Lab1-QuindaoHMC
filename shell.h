#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

//  Constants ─────────────────────────────────────────────
#define MAX_ARGS      256
#define MAX_INPUT_LEN 1024
#define PROMPT        "mysh> "

// Command struct ─────────────────────────────────────────
/*  Populated by the parser, consumed by the executor
 *  Every field has a defined "empty" value 
 * so callers can zero-initialise with init_command() and test fields safely */
typedef struct {
    char *command;            // argv[0]: the program/built-in name     
    char *args[MAX_ARGS];     // full argument vector, NULL-terminated 
    int   arg_count;          // number of entries in args[]            
    char *input_file;         // filename after '<',  or NULL          
    char *output_file;        // filename after '>' / '>>', or NULL     
    bool  append;             // true → '>>'  /  false → '>'           
    bool  background;         // true when line ends with '&'           
} Command;

// interfaces are declared here so that shell.h provides access to all module declarations

// parser.h Interface ────────────────────────────────────
void init_command(Command *cmd);
int  parse_input(char *input, Command *cmd);

// builtins.h Interface ──────────────────────────────────
int  is_builtin(const char *command);
int  run_builtin(const Command *cmd);

// executor.h Interface ──────────────────────────────────
int  execute_command(const Command *cmd);
void reap_zombies(void);

#endif //SHELL_H    // ends shell.h guard