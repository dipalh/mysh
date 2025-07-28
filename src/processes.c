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
#include "jobs.h"
#include "variables.h"
#include "commands.h"
#include "processes.h"

int background_processes = 0;
Node *processes = NULL;
int exit_status = 0;

int process(char **token_arr, size_t token_count) {
    if (token_count >= 1) {
        ssize_t err = assignment(token_arr, token_count, &var_list);
        if (err == 1) { 
            freeTokens(token_arr, token_count);
            return 0;
        }
        bn_ptr builtin_fn = check_builtin(token_arr[0]);
        if (builtin_fn != NULL) {
            ssize_t err = builtin_fn(token_arr, token_count);
            if (err == -1) {
                display_error("ERROR: Builtin failed: ", token_arr[0]);
            }
        } else if (doCommand(token_arr, token_count) != 0) {
            display_error("ERROR: Unknown command: ", token_arr[0]);
        }
    }
    freeTokens(token_arr, token_count);
    return 0;
}



void end_background_processes() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        char pid_str[20];
        sprintf(pid_str, "%d", pid);
        deleteNode(&processes, pid_str);
        background_processes--;

        checkStatus(searchJob(pid));
    }
}

int check_background(char *command){
    char *token_arr[MAX_STR_LEN] = {NULL};
    ssize_t temp = MAX_STR_LEN;
    size_t token_count = tokenize_input(command, token_arr, var_list, &temp);
    int ret = token_count > 0 && strncmp("&", token_arr[token_count - 1], 2) == 0;
    freeTokens(token_arr, token_count);
    return ret;
}

void start_background(char* piped_command[], int* pids, int pipes, ssize_t *max_length, Job *job) {
    char *amper = strchr(piped_command[pipes - 1], '&');
    if (amper != NULL) {
        *amper = '\0';
    }
    (*max_length)--;
    int n = fork();
    if (n == -1) {
        display_error("ERROR: Fork failed", "");
        return;
    } else if (n == 0) {
        dealWithStuff(pipes, piped_command, max_length, pids);
        exit(0);
    } else if (n > 0) {
        pids[0] = n;
        background_processes++;
        char pid_str[20];
        sprintf(pid_str, "%d", n);
        insert(&processes, pid_str, piped_command[0]);
        display_message_format("[%d] %d\n", job->idx, n);
    }
}

void start_foreground(char* piped_command[], int* pids, int pipes, ssize_t *max_length, Job* job){
    if (pipes == 1){
        pids[0] = getpid();
        char *token_arr[MAX_STR_LEN] = {NULL};
        ssize_t temp = MAX_STR_LEN;
        size_t token_count = tokenize_input(piped_command[0], token_arr, var_list, &temp);
        if (token_count > 0 && !strncmp("exit", token_arr[0], 5)){
            freeTokens(token_arr, token_count);
            free(job->command);
            free(job->pids);
            free(job);
            exit_status = 1;
            return;
        }
        process(token_arr, token_count);
        freeJob(job);
        return;
           
    } else {
        dealWithStuff(pipes, piped_command, max_length, pids);
        freeJob(job);
    }
}

int dealWithStuff(ssize_t pipe_count, char** pipe_input_arr, ssize_t *max_length, int *pids) {

    int pipes[pipe_count][2];

    for (ssize_t i = 0; i < pipe_count; i++) {
        if (pipe(pipes[i]) == -1) {
            display_error("ERROR: Pipe failed", "");
            return -1;
        }
    }
    
    for (ssize_t i = 0; i < pipe_count; i++) {
        if (pipe(pipes[i]) == -1) {
            display_error("ERROR: Pipe failed", "");
            return -1;
        }
        
    }
    if (pipe_count == 1) {
        char *token_arr[MAX_STR_LEN] = {NULL};
        ssize_t token_count = tokenize_input(pipe_input_arr[0], token_arr, var_list, max_length);
        
        int n = fork();
        if (n == -1) {
            display_error("ERROR: Fork failed", "");
            return -1;
        }

        if (n == 0){
            if (token_count > 0 && !strncmp("exit", token_arr[0], 5)) {
                freeTokens(token_arr, token_count);
                freeList(var_list);
                return 0;
            }
    
            process(token_arr, token_count);
            exit(0);
        } else {
            pids[0] = n;
        }
        waitpid(pids[0], NULL, 0);
        freeTokens(token_arr, token_count);
        return 0;
    }
    
    for (ssize_t i = 0; i < pipe_count; i++) {
        char *token_arr[MAX_STR_LEN] = {NULL};
        ssize_t token_count = tokenize_input(pipe_input_arr[i], token_arr, var_list, max_length);

        int n = fork();
        if (n == -1) {
            display_error("ERROR: Fork failed", "");
            return -1;
        }

        if (n == 0) { 
            if (i != 0) { 
                dup2(pipes[i-1][0], STDIN_FILENO);
                close(pipes[i-1][0]);
            }
            if (i != pipe_count - 1) { 
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (ssize_t j = 0; j < pipe_count; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (token_count > 0 && !strncmp("exit", token_arr[0], 5)) {
                freeTokens(token_arr, token_count);
                exit(1);
            }
            process(token_arr, token_count);
            exit(0);
        }
        else {
            pids[i] = n;
        }
        freeTokens(token_arr, token_count);
    }

    for (ssize_t i = 0; i < pipe_count; i++) {
        waitpid(pids[i], NULL, 0);

        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    return 0;
}

void freeProcesses(Node* head) {
    while (head != NULL) {
        Node* next = head->next;
        free(head->key);
        free(head->value);
        free(head);
        head = next;
    }
}