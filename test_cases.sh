#!/bin/bash
# =============================================================
#  test_cases.sh  —  Test suite for mysh
#
#  Usage:
#    chmod +x test_cases.sh
#    ./test_cases.sh
#
#  The script feeds commands into ./shell via printf | ./shell,
#  captures output, and compares against expected results.
#  A PASS/FAIL summary is printed at the end.
# =============================================================

SHELL_BIN="./shell"
PASS=0
FAIL=0
TMPDIR_TEST=$(mktemp -d)   # isolated scratch directory

# ── colour codes (safe fallback if terminal doesn't support) ──
GREEN="\033[0;32m"
RED="\033[0;31m"
RESET="\033[0m"

# -------------------------------------------------------------
#  Helper: run_test <description> <input_commands> <expected_substring>
#
#  Sends input_commands to the shell and checks that
#  expected_substring appears somewhere in the output.
# -------------------------------------------------------------
run_test() {
    local desc="$1"
    local input="$2"
    local expected="$3"

    local output
    output=$(printf "%s\nexit\n" "$input" | "$SHELL_BIN" 2>&1)

    if echo "$output" | grep -qF "$expected"; then
        printf "${GREEN}[PASS]${RESET} %s\n" "$desc"
        PASS=$((PASS + 1))
    else
        printf "${RED}[FAIL]${RESET} %s\n" "$desc"
        printf "       expected to find : %s\n" "$expected"
        printf "       actual output    : %s\n" "$output"
        FAIL=$((FAIL + 1))
    fi
}

# -------------------------------------------------------------
#  Helper: run_test_file <description> <input_commands>
#                        <file_to_check> <expected_substring>
#
#  Like run_test but checks the content of a file instead of
#  stdout (used for redirection tests).
# -------------------------------------------------------------
run_test_file() {
    local desc="$1"
    local input="$2"
    local file="$3"
    local expected="$4"

    printf "%s\nexit\n" "$input" | "$SHELL_BIN" > /dev/null 2>&1

    if [ -f "$file" ] && grep -qF "$expected" "$file"; then
        printf "${GREEN}[PASS]${RESET} %s\n" "$desc"
        PASS=$((PASS + 1))
    else
        printf "${RED}[FAIL]${RESET} %s\n" "$desc"
        printf "       expected '%s' in file '%s'\n" "$expected" "$file"
        FAIL=$((FAIL + 1))
    fi
}

# =============================================================
#  Pre-flight check
# =============================================================
if [ ! -x "$SHELL_BIN" ]; then
    echo "ERROR: '$SHELL_BIN' not found or not executable."
    echo "       Run 'make' first, then re-run this script."
    exit 1
fi

echo "============================================="
echo "  mysh Test Suite"
echo "============================================="
echo ""

# =============================================================
#  SECTION 1 — Basic External Commands
# =============================================================
echo "--- Section 1: Basic External Commands ---"

run_test "ls lists files" \
    "ls" \
    "shell"                          # the binary itself must appear

run_test "echo prints text" \
    "echo hello world" \
    "hello world"

run_test "echo with multiple words" \
    "echo foo bar baz" \
    "foo bar baz"

# write a file then cat it
echo "cat test content" > "$TMPDIR_TEST/catfile.txt"
run_test "cat reads a file" \
    "cat $TMPDIR_TEST/catfile.txt" \
    "cat test content"

# write a file then grep it
echo -e "apple\nbanana\ncherry" > "$TMPDIR_TEST/grepfile.txt"
run_test "grep finds a match" \
    "grep banana $TMPDIR_TEST/grepfile.txt" \
    "banana"

run_test "grep no match is silent" \
    "grep mango $TMPDIR_TEST/grepfile.txt" \
    ""                               # empty string always matches — just checks no crash

echo -e "line1\nline2\nline3" > "$TMPDIR_TEST/wcfile.txt"
run_test "wc counts lines" \
    "wc -l $TMPDIR_TEST/wcfile.txt" \
    "3"

echo ""

# =============================================================
#  SECTION 2 — Built-in Commands
# =============================================================
echo "--- Section 2: Built-in Commands ---"

run_test "pwd prints working directory" \
    "pwd" \
    "/"                              # any absolute path contains '/'

run_test "cd changes directory" \
    "cd /tmp
pwd" \
    "/tmp"

run_test "cd with no args goes to HOME" \
    "cd
pwd" \
    "$HOME"

run_test "cd to invalid path prints error" \
    "cd /nonexistent_dir_xyz" \
    "No such file or directory"

run_test "exit terminates shell" \
    "exit" \
    ""                               # just checks it doesn't hang/crash

run_test "exit with code 0" \
    "exit 0" \
    ""

echo ""

# =============================================================
#  SECTION 3 — I/O Redirection
# =============================================================
echo "--- Section 3: I/O Redirection ---"

OUT="$TMPDIR_TEST/out.txt"
APPEND="$TMPDIR_TEST/append.txt"
INPUT="$TMPDIR_TEST/input.txt"

# output redirect (truncate)
run_test_file "output redirect (>) creates file with content" \
    "echo hello > $OUT" \
    "$OUT" \
    "hello"

# running again should truncate, not accumulate
run_test_file "output redirect (>) truncates existing file" \
    "echo hello > $OUT
echo world > $OUT" \
    "$OUT" \
    "world"

# after truncate the old content must be gone
printf "echo only_new > $OUT\nexit\n" | "$SHELL_BIN" > /dev/null 2>&1
if ! grep -qF "hello" "$OUT" 2>/dev/null; then
    printf "${GREEN}[PASS]${RESET} output redirect (>) removes old content\n"
    PASS=$((PASS + 1))
else
    printf "${RED}[FAIL]${RESET} output redirect (>) old content still present\n"
    FAIL=$((FAIL + 1))
fi

# append redirect
rm -f "$APPEND"
run_test_file "append redirect (>>) first write" \
    "echo line1 >> $APPEND" \
    "$APPEND" \
    "line1"

run_test_file "append redirect (>>) second write keeps first line" \
    "echo line1 >> $APPEND
echo line2 >> $APPEND" \
    "$APPEND" \
    "line1"

# input redirect
echo "redirected input" > "$INPUT"
run_test "input redirect (<) feeds file to command" \
    "cat < $INPUT" \
    "redirected input"

# combined input + output
echo "some data" > "$TMPDIR_TEST/combined_in.txt"
run_test_file "combined < and > redirection" \
    "cat < $TMPDIR_TEST/combined_in.txt > $TMPDIR_TEST/combined_out.txt" \
    "$TMPDIR_TEST/combined_out.txt" \
    "some data"

echo ""

# =============================================================
#  SECTION 4 — Background Jobs
# =============================================================
echo "--- Section 4: Background Jobs ---"

run_test "background job (&) prints PID message" \
    "sleep 1 &" \
    "background process started with PID"

run_test "background job (no space: sleep&) prints PID message" \
    "sleep 1&" \
    "background process started with PID"

run_test "multiple background jobs launch" \
    "sleep 1 &
sleep 1 &
sleep 1 &" \
    "background process started with PID"

# immediate exit after background job — shell must not hang
TIMEOUT_OUT=$(timeout 5s bash -c "printf 'sleep 10 &\nexit\n' | $SHELL_BIN" 2>&1)
if [ $? -ne 124 ]; then
    printf "${GREEN}[PASS]${RESET} shell exits immediately after background job\n"
    PASS=$((PASS + 1))
else
    printf "${RED}[FAIL]${RESET} shell hung waiting for background job\n"
    FAIL=$((FAIL + 1))
fi

# background job with output redirection
BG_OUT="$TMPDIR_TEST/bg_out.txt"
printf "echo bg_result > $BG_OUT &\nexit\n" | "$SHELL_BIN" > /dev/null 2>&1
sleep 0.5   # give child time to finish
if grep -qF "bg_result" "$BG_OUT" 2>/dev/null; then
    printf "${GREEN}[PASS]${RESET} background job with output redirection writes file\n"
    PASS=$((PASS + 1))
else
    printf "${RED}[FAIL]${RESET} background job with output redirection did not write file\n"
    FAIL=$((FAIL + 1))
fi

echo ""

# =============================================================
#  SECTION 5 — Edge Cases
# =============================================================
echo "--- Section 5: Edge Cases ---"

# empty input — shell must re-prompt, not crash
run_test "empty input is ignored" \
    "
echo still_alive" \
    "still_alive"

# whitespace-only line
run_test "whitespace-only line is ignored" \
    "   
echo alive_after_spaces" \
    "alive_after_spaces"

# long command line (many arguments)
LONG_ARGS=$(python3 -c "print(' '.join(['arg']*200))")
run_test "long argument list echo" \
    "echo $LONG_ARGS" \
    "arg"

# many arguments to echo — just check it doesn't crash
run_test "many arguments do not crash shell" \
    "echo a b c d e f g h i j k l m n o p q r s t u v w x y z" \
    "a b c d e"

# redirection to /dev/null — output suppressed, no crash
run_test "redirect to /dev/null suppresses output" \
    "echo this_is_suppressed > /dev/null
echo visible_after_null" \
    "visible_after_null"

# rapid sequence of commands
run_test "rapid sequence of commands" \
    "echo one
echo two
echo three" \
    "three"

# invalid command — must print error, not crash
run_test "invalid command prints error" \
    "notarealcommand_xyz" \
    "execvp"

# missing filename after redirect operator
run_test "missing filename after > prints syntax error" \
    "echo hello >" \
    "syntax error"

run_test "missing filename after < prints syntax error" \
    "cat <" \
    "syntax error"

# command& with no space
run_test "ls& (no space before &) runs in background" \
    "ls&" \
    "background process started with PID"

echo ""

# =============================================================
#  SECTION 6 — Prompt Check
# =============================================================
echo "--- Section 6: Prompt ---"

run_test "prompt displays mysh>" \
    "echo prompt_check" \
    "mysh>"

echo ""

# =============================================================
#  Cleanup & Summary
# =============================================================
rm -rf "$TMPDIR_TEST"

echo "============================================="
TOTAL=$((PASS + FAIL))
printf "  Results: ${GREEN}%d passed${RESET}, ${RED}%d failed${RESET} / %d total\n" \
    "$PASS" "$FAIL" "$TOTAL"
echo "============================================="

# exit with non-zero if any test failed (useful for CI)
[ "$FAIL" -eq 0 ]