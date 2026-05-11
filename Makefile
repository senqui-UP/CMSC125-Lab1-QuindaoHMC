# ─────────────────────────────────────────────
#  Makefile for mysh – a simplified Unix shell
# ─────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = shell
SRCS    = main.c parser.c executor.c builtins.c
OBJS    = $(SRCS:.c=.o)

# Default target: compile everything
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pattern rule: compile each .c to a .o
%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

# Remove all build artefacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean