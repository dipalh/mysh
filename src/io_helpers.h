#ifndef __IO_HELPERS_H__
#define __IO_HELPERS_H__

#include <sys/types.h>
#include "linkedlist.h"

#define MAX_STR_LEN 128
#define DELIMITERS " \t\n"


/* Prereq: pre_str, str are NULL terminated string
 */
void display_message(char *str);
void display_message_format(const char *format, ...);
void display_error(char *pre_str, char *str);
void freeTokens(char **token_arr, size_t token_count);


/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr);

size_t pipe_input(char *in_ptr, char **piped_input, ssize_t *max_length);

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens, Node *variables, ssize_t *max_length);


#endif
