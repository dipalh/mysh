#ifndef PROCESSES_H
#define PROCESSES_H

#include <stdio.h>
#include "linkedlist.h"
#include "jobs.h"

extern int background_processes;
extern Node *processes;
extern int exit_status;

int process(char **token_arr, size_t token_count);
// int start_background_process(char **token_arr, size_t token_count);
void end_background_processes();

int check_background(char* command);
void start_background(char* piped_command[], int* pids, int pipes, ssize_t *max_length, Job* job);
void start_foreground(char* piped_command[], int* pids, int pipes, ssize_t *max_length, Job* job);

int dealWithStuff(ssize_t pipe_count, char** pipe_input_arr, ssize_t *max_length, int *pids);
void freeProcesses(Node* head);
#endif