#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "io_helpers.h"
#include "variables.h"

// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
}

void display_message_format(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[MAX_STR_LEN];
    vsnprintf(buffer, MAX_STR_LEN, format, args);
    va_end(args);
    display_message(buffer);
}

/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(char *pre_str, char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}

void freeTokens(char **token_arr, size_t token_count) {
    for (size_t i = 0; i < token_count; i++){
        free(token_arr[i]);
    }
}


// ===== Input tokenizing =====

/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1);
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }
    if (read_len > MAX_STR_LEN) {
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
        int junk = 0;
        while((junk = getchar()) != EOF && junk != '\n');
    }
    in_ptr[read_len] = '\0';
    return retval;
}

size_t pipe_input(char *in_ptr, char **piped_input, ssize_t *max_length){
    size_t pipe_count = 0;
    char *remainder = in_ptr;
    char *curr_ptr = strtok_r(remainder, "|", &remainder);
    while (curr_ptr != NULL) {
        piped_input[pipe_count] = curr_ptr;
        *max_length -= 1;
        pipe_count++;
        curr_ptr = strtok_r(remainder, "|", &remainder);
    }
    return pipe_count;
}
/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens, Node *variables, ssize_t *max_length) {
    char *remainder = in_ptr;
    char *curr_ptr = strtok_r(remainder, DELIMITERS, &remainder);
    size_t token_count = 0;

    while (curr_ptr != NULL && *max_length > 0) {
	    tokens[token_count] = expansion(curr_ptr, variables, *max_length);
        *max_length -= strlen(tokens[token_count]) + 1;
        token_count++;

	    curr_ptr = strtok_r(remainder, DELIMITERS, &remainder);
    }
    tokens[token_count] = NULL;
    return token_count;
}