#include "variables.h"

#include "builtins.h"
#include "io_helpers.h"

Node *var_list = NULL;

ssize_t assignment(char **token_arr, size_t token_count, Node **variables){
    if (strchr(token_arr[0], '=') != NULL) {
        if (token_count > 1){
             display_error("ERROR: Assignment Failed due to too many arguments.", "");
             return -1;
        }
        else if (token_arr[0][0] == '='){
             display_error("ERROR: Assignment Failed due to no key.", "");
             return -1;
        }
        else{
            char* key = strtok(token_arr[0], "=");
            char* value = strtok(NULL, "");

            if (key && value) {
                insert(variables, key, value);
            }
            return 1;
        }
     }
     return 0;
}

char *expansion(char *token, Node *variables, size_t max_length){
    char *expanded_token = (char *)malloc(max_length);
    if (expanded_token == NULL) {
        display_error("ERROR: Memory allocation failed.", "");
        return NULL;
    }
    expanded_token[0] = '\0';

    char *curr_ptr;
    char *remainder = token;

    if (token[0] != '$') {
        curr_ptr = strsep(&remainder, "$");
        if (curr_ptr) {
            strncat(expanded_token, curr_ptr, max_length);
            max_length -= strlen(curr_ptr);
        }
    } else if ((curr_ptr = strsep(&remainder, "$")) && strlen(curr_ptr) == 1) {
        strncat(expanded_token, "$", 2);
        max_length -= 1;
    } 

    while ((curr_ptr = strsep(&remainder, "$")) != NULL && max_length > 0) {
        if (strlen(curr_ptr) == 0) {
            strncat(expanded_token, "$", 2);
            max_length -= 1;
        } else {
            Node *node = searchNode(variables, curr_ptr);
            if (node) {
                strncat(expanded_token, node->value, max_length - 1);
                max_length -= strlen(node->value);
            }
        }
    }

    return expanded_token;
}