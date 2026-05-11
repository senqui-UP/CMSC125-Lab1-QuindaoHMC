// Input parsing: tokenizing, detecting >, >>, <, &, filling the Command struct

#include "shell.h"

// init_command ─────────────────────────────────────────────
//  initialises all fields to 0 so that executor can safely test any field
void init_command(Command *cmd)
{
    cmd->command     = NULL;
    cmd->arg_count   = 0;
    cmd->input_file  = NULL;
    cmd->output_file = NULL;
    cmd->append      = false;
    cmd->background  = false;

    for (int i = 0; i < MAX_ARGS; i++)
        cmd->args[i] = NULL;
}

// strip_backround ─────────────────────────────────────────────
/*  cases for when tokens are command&, so that they can be read as the command
 *  replaces trailing & with '\0', returns 1 to signal that the background flag should be set
 *  exactly "&" tokens are left unchanged, returns 0 — the main loop handles that case itself */
static int strip_background(char *token)
{
    size_t len = strlen(token);
    if (len > 1 && token[len - 1] == '&') {
        token[len - 1] = '\0';   /* null-terminate before the '&' */
        return 1;
    }
    return 0;
}

// parse_input ─────────────────────────────────────────────
//  Tokenises `input` and fills *cmd struct
int parse_input(char *input, Command *cmd)
{
    init_command(cmd);

    // Tokenise by whitespace
    char *token = strtok(input, " \t\n");

    // if whitespace
    if (token == NULL)
        return -1;

    // checks if token is a recognized operator
    while (token != NULL) {

        // strip trailing
        if (strip_background(token))
            cmd->background = true;

        // >> Output redirect (Append)
        if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "mysh: syntax error: expected filename after '>>'\n");
                return -2;
            }
            cmd->output_file = token;
            cmd->append      = true;

        // > Output redirect (Truncate)
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "mysh: syntax error: expected filename after '>'\n");
                return -2;
            }
            cmd->output_file = token;
            cmd->append      = false;

        // < Input Redirect
        } else if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "mysh: syntax error: expected filename after '<'\n");
                return -2;
            }
            cmd->input_file = token;

        // & Background Flag
        // must be the last meaningful token
        // keeps it looping so that any trailing whitespace terminates naturally
        } else if (strcmp(token, "&") == 0) {
            cmd->background = true;
        
        // regular arguments / commands
        } else {
            if (cmd->arg_count >= MAX_ARGS - 1) {
                fprintf(stderr, "mysh: too many arguments (max %d)\n", MAX_ARGS - 1);
                return -2;
            }
            cmd->args[cmd->arg_count++] = token;
        }

        token = strtok(NULL, " \t\n");
    }

    // args[] must be NULL-terminated for execvp
    cmd->args[cmd->arg_count] = NULL;

    // only operators and no command
    if (cmd->arg_count == 0) {                        
        fprintf(stderr, "mysh: syntax error: no command specified\n");
        return -2;
    }

    // first argument is the command name
    cmd->command = cmd->args[0];

    return 0;
}