
#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdio.h>
#include "linkedlist.h"

extern Node *var_list;

ssize_t assignment(char **token_arr, size_t token_count, Node **variables);
char *expansion(char *token, Node *variables, size_t max_length);

#endif