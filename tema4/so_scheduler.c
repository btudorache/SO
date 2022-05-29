#include <pthread.h>
#include <semaphore.h>

#include "so_scheduler.h"

#define NUM_THREADS 20

#define PREEMPTED 1
#define NOT_PREEMPTED 0

#define NEW 1
#define READY 2
#define RUNNING 3
#define WAITING 4
#define TERMINATED 5

unsigned int assigned_time_quantum;
unsigned int num_io_events;

int num_threads;
int running_thread_index;
unsigned int running_thread_quantum;
unsigned int running_thread_priority;

// THREAD STATUSES:
// NEW 1
// READY 2
// RUNNING 3
// WAITING 4
// TERMINATED 5
unsigned int thread_status[NUM_THREADS];
unsigned int thread_priority[NUM_THREADS];
pthread_t thread_ids[NUM_THREADS];
sem_t semaphores[NUM_THREADS];

typedef struct planned_thread_params {
    int thread_id;
    int priority;
    so_handler* running_func;
} thread_param;

// status == 0 => not preempted 
// status == 1; => preempted with id thread_id
typedef struct planner_result {
    int thread_id;
    unsigned char status;
} planner_res;

void planner_add_new_thread() {

}

planner_res planner_check_status() {
    planner_res result;
    result.status = NOT_PREEMPTED;
    result.thread_id = running_thread_index;
    return result;
}
 
void planner_terminated_thread() {

}

void* start_thread(void* params) {
    thread_param* param = (thread_param*)params;

    sem_wait(&semaphores[param->thread_id]);
    param->running_func(param->priority);

    thread_status[param->thread_id] = TERMINATED;
    // start new thread
    planner_terminated_thread();
    return NULL;
}

int so_init(unsigned int time_quantum, unsigned int io) {
    assigned_time_quantum = time_quantum;
    num_io_events = io;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        sem_init(&semaphores[i], 0, 0);
    }

    return 0;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
    thread_param params;
    params.thread_id = num_threads;
    params.priority = priority;
    params.running_func = func;

    thread_status[num_threads] = NEW;
    pthread_create(&thread_ids[num_threads], NULL, start_thread, (void *)&params);
    num_threads++;
    planner_add_new_thread();

    return thread_ids[num_threads - 1];
}

int so_wait(unsigned int io) {
    return 0;
}

int so_signal(unsigned int io) {
    return 0;
}

void so_exec() {
    planner_res res = planner_check_status();
    if (res.status == PREEMPTED) {
        sem_wait(&semaphores[res.thread_id]);
    }
}

void so_end() {
    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        sem_destroy(&semaphores[i]);
    }
}