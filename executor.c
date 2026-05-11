#include "shell.h"

// reap_zombies ─────────────────────────────────────────────
/*  Called at top of every prompt loop to collect any finished background children
 *  WNOHANG = no wait — if no child is ready, return immediately and keep shell responsive
 *  Prevents finished background processes from lingering in process table */
void reap_zombies(void)
{
    int   status;
    pid_t pid;

    // loop until there's no more finished children to collect
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[mysh] background process %d finished", pid);
        if (WIFEXITED(status))
            printf(" (exit status %d)", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf(" (killed by signal %d)", WTERMSIG(status));
        printf("\n");
    }
}

// execute_command ──────────────────────────────────────────
/*  forks a child
 *  sets up I/O redirection via dup2
 *  exec's the requested command
 *  parent = waits (foreground) or returns immediately (background)

 *  CRITICAL TIMING:
 *    open()  → must happen in child, after fork
 *    dup2()  → must happen after fork, before execvp
 *    close() → must happen right after dup2,
 *              before execvp, so the old fd is
 *              not leaked into the new image */
int execute_command(const Command *cmd)
{
    pid_t pid = fork();

    // if fork failed, stay in parent and report the error
    if (pid < 0) {
        perror("mysh: fork");
        return -1;
    }

    // CHILD process ----------------------------------------
    if (pid == 0) {

        // < input redirection
        if (cmd->input_file != NULL) {
            int fd_in = open(cmd->input_file, O_RDONLY);
            if (fd_in < 0) {
                fprintf(stderr, "mysh: cannot open '%s' for reading: %s\n",
                        cmd->input_file, strerror(errno));
                exit(1);
            }
            if (dup2(fd_in, STDIN_FILENO) < 0) {
                perror("mysh: dup2 (stdin)");
                exit(1);
            }
            close(fd_in);               // original fd no longer needed
        }

        // > or >> output redirection
        if (cmd->output_file != NULL) {
            int flags = O_WRONLY | O_CREAT |
                        (cmd->append ? O_APPEND : O_TRUNC);
            int fd_out = open(cmd->output_file, flags, 0644);
            if (fd_out < 0) {
                fprintf(stderr, "mysh: cannot open '%s' for writing: %s\n",
                        cmd->output_file, strerror(errno));
                exit(1);
            }
            if (dup2(fd_out, STDOUT_FILENO) < 0) {
                perror("mysh: dup2 (stdout)");
                exit(1);
            }
            close(fd_out);              // original fd no longer needed
        }

        // replace child image with the requested program
        execvp(cmd->command, cmd->args);
        // execvp only returns on failure
        perror("mysh: execvp");
        exit(127);
    }

    // PARENT process ---------------------------------------
    // background: print the PID and return immediately
    if (cmd->background) {
        printf("[mysh] background process started with PID %d\n", pid);
        return 0;
    }

    // foreground: block until child finishes
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("mysh: waitpid");
        return -1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);          // bash convention

    return -1;
}