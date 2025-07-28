#include "commands.h"
#include "io_helpers.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

/*
 * Executes a command with the given tokens.
 * Prereq: tokens is an array of NULL terminated strings, token_count is the number of tokens.
 * Returns: 0 on success, -1 on failure.
 */
ssize_t doCommand(char **tokens, size_t token_count){
    if (token_count < 1) {
        display_error("ERROR: No command provided", "");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        display_error("ERROR: Fork failed", "");
        return -1;
    }

    if (pid == 0) {
        char path[256] = "/usr/bin/";
        strncat(path, tokens[0], sizeof(path) - strlen(path) - 1);
        execvp(path, tokens);
        // display_error("ERROR: Command failed: ", tokens[0]);
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 0;
}