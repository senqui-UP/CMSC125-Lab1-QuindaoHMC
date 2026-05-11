#include "shell.h"

// is_builtin ─────────────────────────────────────────────
/*  Returns 1 if `command` is a built-in that  must run inside the shell process
 *  Returns 0 otherwise
 *  bypasses fork/exec because child processes cannot modify the parent's environment */
int is_builtin(const char *command)
{
    if (command == NULL) return 0;

    return (strcmp(command, "exit") == 0 ||
            strcmp(command, "cd")   == 0 ||
            strcmp(command, "pwd")  == 0);
}

// run_builtin ────────────────────────────────────────────
/*  Dispatches to the correct built-in handler.
 *  Returns 0 on success, non-zero on error.
 *  Return -99 = sentinel telling main() that the shell should exit */
int run_builtin(const Command *cmd)
{
    // exit
    if (strcmp(cmd->command, "exit") == 0) {
        int status = 0;
        if (cmd->arg_count >= 2)                // optional exit code
            status = atoi(cmd->args[1]);
        exit(status);                           // shell termination
    }

    // pwd
    if (strcmp(cmd->command, "pwd") == 0) {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("mysh: pwd");
            return 1;
        }
        printf("%s\n", cwd);
        return 0;
    }

    // cd
    if (strcmp(cmd->command, "cd") == 0) {
        const char *target;

        // 'cd' with no arguments → go to HOME
        if (cmd->arg_count < 2) {
            target = getenv("HOME");
            if (target == NULL) {
                fprintf(stderr, "mysh: cd: HOME not set\n");
                return 1;
            }
        } else {
            target = cmd->args[1];
        }

        if (chdir(target) != 0) {
            perror("mysh: cd");
            return 1;
        }
        return 0;
    }

    // failsafe for is_builtin, should be unreachable
    fprintf(stderr, "mysh: unknown built-in: %s\n", cmd->command);
    return 1;
}