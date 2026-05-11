# A Simplified Unix Shell
### CMSC 125-1 Lab-1 Laboratory 1 | Quindao, Hansen Maeve C.

`mysh` is a simplified Unix shell developed in C using the POSIX API. It demonstrates core operating systems concepts including process management, file descriptor manipulation, I/O redirection, and background execution. The shell presents an interactive prompt, parses user input, and dispatches commands either to built-in handlers or to child processes created via `fork`/`exec`.

---

## Compilation and Setup Instructions

A `Makefile` is provided. From the project root directory, run:

```bash
make
```
To remove all compiled files and start fresh:

```bash
make clean
```

Launch the shell interactively:

```bash
./shell
```

The prompt `mysh>` will appear. Type any command and press Enter. Examples of commands are:

```
mysh> echo hello world
mysh> pwd
mysh> cd /tmp
mysh> ls
mysh> exit
```

---

## Features

### Basic External Command Execution
Any program on the system PATH can be run directly.
```
mysh> ls -la
mysh> cat file.txt
mysh> grep hello file.txt
mysh> wc -l file.txt
```

### Built-in Commands
These run inside the shell process itself and do not fork a child.

| Command | Behaviour |
|---------|-----------|
| `exit` | Exits the shell. Accepts an optional exit code: `exit 1` |
| `cd <path>` | Changes the working directory. `cd` alone goes to `$HOME` |
| `pwd` | Prints the current working directory |

### I/O Redirection

| Operator | Behaviour |
|----------|-----------|
| `>` | Redirect stdout to a file (truncates if file exists) |
| `>>` | Redirect stdout to a file (appends if file exists) |
| `<` | Redirect stdin from a file |

Examples:
```
mysh> echo hello > out.txt
mysh> echo world >> out.txt
mysh> cat < out.txt
mysh> cat < input.txt > output.txt
```

### Background Execution
Append `&` to run a command in the background. The shell prints the PID and returns the prompt immediately. Both spaced and unspaced forms are supported.
```
mysh> sleep 10 &
[mysh] background process started with PID 4821
mysh> sleep 10&
[mysh] background process started with PID 4822
mysh> ls&
[mysh] background process started with PID 4823
```
When a background process finishes, the shell reports it at the next prompt:
```
mysh> [mysh] background process 4821 finished (exit status 0)
```

---

## Project Structure

```
.
├── main.c          — Interactive prompt loop (REPL), zombie reaping, dispatch
├── parser.c        — Input tokenisation, operator detection, Command struct population
├── builtins.c      — Built-in command implementations: cd, pwd, exit
├── executor.c      — Process creation (fork/exec), I/O redirection (dup2), wait/waitpid
├── shell.h         — Shared header: Command struct definition, all function prototypes
├── Makefile        — Build system: `all` and `clean` targets
├── test_cases.sh   — Automated test suite (36 tests across 6 sections)
└── README.md       — This file
```

The provided Makefile compiles the source with -Wall -Wextra flags to ensure type safety and rigorous error checking during the build process

### The Command Struct

The central data structure passed between the parser and executor:

```c
typedef struct {
    char *command;         // argv[0]: the program or built-in name
    char *args[256];       // full argument vector, NULL-terminated for execvp
    int   arg_count;       // number of entries in args[]
    char *input_file;      // filename after '<', or NULL
    char *output_file;     // filename after '>' or '>>', or NULL
    bool  append;          // true = '>>'  /  false = '>'
    bool  background;      // true when line ends with '&'
} Command;
```

---

## Testing

Run the automated test suite:

```bash
chmod +x test_cases.sh
./test_cases.sh
```

The script feeds commands into the shell via stdin and validates output. Results are colour-coded PASS/FAIL with a summary at the end.

### What the Tests Cover

| Section | Tests | What is Validated |
|---------|-------|-------------------|
| 1 — Basic External Commands | 7 | `ls`, `echo`, `cat`, `grep`, `wc` |
| 2 — Built-in Commands | 6 | `cd`, `pwd`, `exit`, error on bad path |
| 3 — I/O Redirection | 7 | `>`, `>>`, `<`, combined, truncation |
| 4 — Background Jobs | 5 | `&` with space, `&` without space, multiple jobs, immediate exit, bg with redirection |
| 5 — Edge Cases | 10 | Empty input, whitespace lines, long args, `/dev/null`, rapid sequences, invalid commands, malformed operators |
| 6 — Prompt | 1 | `mysh>` appears correctly |
| **Total** | **36** | |

---

## Technical Design Choices

### Why `cd` is a Built-in
If the shell forked a child process to run `cd`, the directory change would only affect that child. The parent shell process would remain in the original directory. Built-in commands must therefore run directly inside the shell process without forking, which is why `is_builtin()` is checked before any call to `fork()`.

### The `dup2` Timing Window
File descriptor redirection via `dup2()` is performed strictly after `fork()` but before `execvp()`. This is the only window where it is correct: after the fork so the parent's descriptors are unaffected, and before exec so the new process image inherits the redirected descriptors. The redirection logic involves closing the standard streams (0 for STDIN, 1 for STDOUT) and using dup2() to clone the file descriptor of the opened file into the standard stream slot before the execvp call.

### Zombie Process Prevention
Background processes are reaped at the top of every prompt loop using `waitpid(-1, &status, WNOHANG)`. The `WNOHANG` flag means the call returns immediately if no child has finished, keeping the shell responsive. Without this, terminated background processes would remain in the process table as zombies until the shell itself exited.

### No-Space Background Operator (`command&`)
Standard shells accept both `sleep 5 &` and `sleep 5&`. Since `strtok` splits on whitespace only, `sleep5&` arrives as a single token and would never match a standalone `"&"` comparison. A `strip_background()` helper pre-processes every token before the operator checks run: if a token's last character is `&` and the token is longer than one character, the `&` is stripped in place and the background flag is set. Standalone `"&"` is left for the existing branch to handle.

### Error Reporting with `perror`
All system call failures (`fork`, `dup2`, `waitpid`, `open`) use `perror()` to print the OS-level error string (e.g. `Permission denied`, `No such file or directory`) rather than a generic message. The `execvp` failure path in particular benefits from this — it correctly distinguishes between a missing binary and a permission error.

---

## Architectural Scope and Limitations

- Interactive features like **command history (via readline) were excluded** to focus the implementation on core kernel-level interactions like signal handling and process control
- **No quoted string support.** The parser splits on whitespace using `strtok`, so paths or arguments containing spaces (e.g. `cd My Documents`) are not handled correctly. Each space-separated word is treated as a separate token.
- **No `cd -` support.** Returning to the previous directory is not implemented (`OLDPWD` is not tracked).
- **Input line length is capped at 1024 characters.** Lines longer than this are silently truncated by `fgets`.
- **No environment variable expansion.** Tokens like `$HOME` or `$PATH` are passed literally to commands and are not expanded by the shell.
- **No signal forwarding.** `SIGINT` (Ctrl-C) currently terminates the entire shell rather than only the foreground child process.