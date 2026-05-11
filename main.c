#include "shell.h"

/* ─────────────────────────────────────────────
 *  handles entry point, main shell loop, and zombie reaping
 *    - reap any finished background children
 *    - print prompt and read a line of input
 *    - parse the line into a Command struct
 *    - dispatch to built-in or external handler
 *    - repeat forever (exit built-in calls exit())
 * ───────────────────────────────────────────── */

int main(void)
{
    char input[MAX_INPUT_LEN];
    Command cmd;

    while (1) {

        // collect any finished background jobs
        reap_zombies();

        // display prompt
        printf(PROMPT);
        fflush(stdout);                     // ensure prompt appears before blocking on read

        // read one line of input ─
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // EOF (Ctrl-D): exit gracefully
            printf("\n");
            break;
        }

        // parse: tokenise and fill Command struct
        int parse_result = parse_input(input, &cmd);

        // reprompt for invalid commands
        if (parse_result == -1)             // empty / whitespaces
            continue;
        if (parse_result == -2)             // syntax error, already printed
            continue;

        // dispatch
        if (is_builtin(cmd.command))
            run_builtin(&cmd);
        else
            execute_command(&cmd);
    }

    return 0;
}