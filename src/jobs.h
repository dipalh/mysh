#ifndef JOBS_H
#define JOBS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Job {
    int pipes;
    char *command;
    int *pids;
    int status;
    int idx;
    int background;
    struct Job* next;
} Job;

extern int job_count;
extern Job* jobs;
extern Job* finished_jobs;
extern int finished_job_count;

Job* createJob(char* command);
int insertJob(char* command);
Job* searchJob(int pid);
void finishJob(Job* job);
int checkStatus(Job* job);
void printFinishedJobs();
void cleanFinishedJobs();
void deleteJob(int idx);
Job *lastJob();
void trimJobs();
int max_index();
void freeJobs();
void freeJob(Job* job);

#endif
