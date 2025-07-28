#include "jobs.h"
#include "io_helpers.h"
#include "variables.h"
#include "processes.h"


int job_count = 0;
Job* jobs = NULL;
Job* finished_jobs = NULL;
int finished_job_count = 0;

/*
 * Creates a new job node with the given command.
 * Starts the job in the background or foreground based on the command.
 * Returns a pointer to the new job node.
 * If memory allocation fails, it displays an error message and returns NULL.
 */
Job* createJob(char* command) {
    Job* newNode = (Job*)malloc(sizeof(Job));
    if (newNode == NULL) {
        display_error("Memory allocation failed!", "");
        return NULL;
    }
    newNode->idx = max_index() + 1;
    job_count++;
    newNode->command = (char*)malloc(sizeof(char) * 129);
    if (newNode->command == NULL) {
        display_error("Memory allocation failed!", "");
        return NULL;
    }
    strncpy(newNode->command, command, 129);
    newNode->status = 0;
    newNode->next = NULL;
    char *piped_command[MAX_STR_LEN] = {NULL};
    ssize_t max_length = MAX_STR_LEN + 1;
    newNode->pipes = pipe_input(command, piped_command, &max_length);
    newNode->pids = (int*)malloc(sizeof(int*) * newNode->pipes);

    if (newNode->pipes == 0) {
        return newNode;
    }

    char* copy = strdup(piped_command[newNode->pipes - 1]);

    newNode->background = check_background(copy);

    free(copy);

    if (newNode->background) {
        start_background(piped_command, newNode->pids, newNode->pipes, &max_length, newNode);
        return newNode;
    } else {
        start_foreground(piped_command, newNode->pids, newNode->pipes, &max_length, newNode);
        return NULL;
    }

    return newNode;
}

/*
 * Inserts a new job into the job list.
 * Returns 0 on success, -1 on failure.
 */
int insertJob(char* command) {
    if (jobs == NULL) {
	    Job* newNode = createJob(command);

        jobs = newNode;

        return 0;
    }
    Job* temp = jobs;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    Job* newNode = createJob(command);
    temp->next = newNode;
    return 0;
}

/*
 * Searches for a job by its process ID (PID).
 * Returns a pointer to the job if found, NULL otherwise.
 */
Job* searchJob(int pid){
    Job* temp = jobs;
    while (temp != NULL) {
        for (int i = 0; i < temp->pipes; i++) {
            if (temp->pids[i] == pid) {
                return temp;
            }
        }
        temp = temp->next;
    }
    return NULL;
}

/*
 * Finishes a job by moving it to the finished jobs list.
 */
void finishJob(Job* job){
    if (finished_jobs == NULL){
        finished_jobs = job;
    } else {
        Job* temp = finished_jobs;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = job;
    }
    finished_job_count++;
}

/*
 * Checks the status of a job.
 * Returns 0 if the job is still running, 1 if it has finished.
 */
int checkStatus(Job* job) {
    if (job == NULL) {
        return 0;
    }

    for (int i = 0; i < job->pipes; i++) {
        char pid[20];
        snprintf(pid, 20, "%d", job->pids[i]);
        if (searchNode(processes, pid)) {
            return 0;
        }
    }

    finishJob(job);
    deleteJob(job->idx);
    return 1;
}

/*
 * Prints all finished jobs.
 */
void printFinishedJobs(){
    Job* temp = finished_jobs;
    int idx = 1;
    while (temp != NULL) {
        if (idx == finished_job_count){
            display_message_format("[%d]+  Done %s", temp->idx, temp->command);
        } else if (idx == finished_job_count - 1){
            display_message_format("[%d]-  Done %s", temp->idx, temp->command);
        } else {
            display_message_format("[%d]   Done %s", temp->idx, temp->command);
        }
        idx++;
        temp = temp->next;
    }
    cleanFinishedJobs();
}

/*
 * Cleans up all finished jobs.
 */
void cleanFinishedJobs(){
    while (finished_jobs != NULL) {
        Job* next = finished_jobs->next;
        free(finished_jobs->command);
        free(finished_jobs->pids);
        free(finished_jobs);
        finished_jobs = next;
    }
    finished_job_count = 0;
}

/*
 * Deletes a job from the job list.
 */
void deleteJob(int idx){
    Job* temp = jobs;
    Job* prev = NULL;
    while (temp != NULL) {
        if (temp->idx == idx){
            if (prev == NULL){
                jobs = temp->next;
            } else {
                prev->next = temp->next;
            }
            temp->next = NULL;
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}

/*
 * frees all jobs from memory.
 */
void freeJobs(){
    while (jobs != NULL) {
        Job* next = jobs->next;
        freeJob(jobs);
        jobs = next;
    }
}

/*
 * Frees a job from memory.
 */
void freeJob(Job* job){
    free(job->command);
    free(job->pids);
    free(job);
}
