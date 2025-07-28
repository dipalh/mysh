#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <signal.h>
#include <unistd.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "commands.h"
#include "processes.h"
#include "jobs.h"

char *prompt = "mysh$ ";

void sigint_handler(__attribute__((unused)) int sig) {
    if (!in_client_mode) {
        display_message("\n");
        display_message(prompt);
    }
}
int main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[]) {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, end_background_processes);
    
    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';

    while (1) {
        display_message(prompt);

        ssize_t ret = get_input(input_buf);

        printFinishedJobs();

        if (ret == 0) {
            freeJobs();
            freeProcesses(processes);
            freeList(var_list);
            if (server_pid > 0) {
                bn_close_server(NULL, 0);
            }
            return 0;
        }

        insertJob(input_buf);
        if (exit_status == 1) {
            freeJobs();
            freeProcesses(processes);
            freeList(var_list);
            if (server_pid > 0) {
                bn_close_server(NULL, 0);
            }
            return 0;
        }
    }
    return 0;
}
