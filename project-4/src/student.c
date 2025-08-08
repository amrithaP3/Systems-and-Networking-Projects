/*
 * student.c
 * Multithreaded OS Simulation for CS 2200
 * Fall 2024
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "student.h"
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);

static unsigned int cpu_count;

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 *
 * rq is a pointer to a struct you should use for your ready queue
 * implementation. The head of the queue corresponds to the process
 * that is about to be scheduled onto the CPU, and the tail is for
 * convenience in the enqueue function. See student.h for the
 * relevant function and struct declarations.
 *
 * Similar to current[], rq is accessed by multiple threads,
 * so you will need to use a mutex to protect it. ready_mutex has been
 * provided for that purpose.
 *
 * The condition variable queue_not_empty has been provided for you
 * to use in conditional waits and signals.
 *
 * Please look up documentation on how to properly use pthread_mutex_t
 * and pthread_cond_t.
 *
 * A scheduler_algorithm variable and sched_algorithm_t enum have also been
 * supplied to you to keep track of your scheduler's current scheduling
 * algorithm. You should update this variable according to the program's
 * command-line arguments. Read student.h for the definitions of this type.
 */
static pcb_t **current;
static queue_t *rq;

static pthread_mutex_t current_mutex;
static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;

static sched_algorithm_t scheduler_algorithm;
static unsigned int cpu_count;
static unsigned int age_weight;

static int timeslice = -1;

/** ------------------------Problem 3-----------------------------------
 * Checkout PDF Section 5 for this problem
 * 
 * priority_with_age() is a helper function to calculate the priority of a process
 * taking into consideration the age of the process.
 * 
 * It is determined by the formula:
 * Priority With Age = Priority - (Current Time - Enqueue Time) * Age Weight
 * 
 * @param current_time current time of the simulation
 * @param process process that we need to calculate the priority with age
 * 
 */
extern double priority_with_age(unsigned int current_time, pcb_t *process) {
    return (process->priority - (current_time - process->enqueue_time) * age_weight);

}

/** ------------------------Problem 0 & 3-----------------------------------
 * Checkout PDF Section 2 and 5 for this problem
 * 
 * enqueue() is a helper function to add a process to the ready queue.
 * 
 * NOTE: For Priority, FCFS, and SRTF scheduling, you will need to have additional logic
 * in this function and/or the dequeue function to account for enqueue time
 * and age to pick the process with the smallest age priority.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 *
 * @param queue pointer to the ready queue
 * @param process process that we need to put in the ready queue
 */
void enqueue(queue_t *queue, pcb_t *process)
{   
    // Locking the queue
    pthread_mutex_lock(&queue_mutex);

    if (is_empty(queue)) {
        queue->head = process;
        queue->tail = process;
    } else {
        queue->tail->next = process;
        queue->tail = process;
    }
    process->enqueue_time = get_current_time();

    // Signaling that queue is not empty 
    // Need to signal BEFORE unlocking the mutex to avoid race conditions!
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);
}

/**
 * dequeue() is a helper function to remove a process to the ready queue.
 *
 * NOTE: For Priority, FCFS, and SRTF scheduling, you will need to have additional logic
 * in this function and/or the enqueue function to account for enqueue time
 * and age to pick the process with the smallest age priority.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 * 
 * @param queue pointer to the ready queue
 */
pcb_t *dequeue(queue_t *queue)
{
    pthread_mutex_lock(&queue_mutex);

    if (is_empty(queue)) {
        pthread_mutex_unlock(&queue_mutex);
        return NULL;
    }

    if (scheduler_algorithm == PA) {
        pcb_t *curr = queue->head;
        pcb_t *prev = NULL;
        
        pcb_t *chosen_prev = NULL;
        pcb_t *chosen = queue->head;
        double max_priority = priority_with_age(get_current_time(), chosen);
        // Finding the right process to dequeue according to priority with age
        while (curr != NULL) {
            if (priority_with_age(get_current_time(), curr) < max_priority) {
                chosen_prev = prev;
                chosen = curr;
                max_priority = priority_with_age(get_current_time(), curr);
            }
            prev = curr;
            curr = curr->next;
        }

        if (chosen_prev == NULL) {
            // Dequeueing process at head
            pcb_t *process = queue->head;
            queue->head = queue->head->next;
            if (queue->tail == process) {
                queue->tail = NULL;
            }
            process->next = NULL;
            pthread_mutex_unlock(&queue_mutex);
            return process;
        } else {
            chosen_prev->next = chosen->next;
            if (queue->tail == chosen) {
                queue->tail = chosen_prev;
            }
            chosen->next = NULL;
            pthread_mutex_unlock(&queue_mutex);
            return chosen;
        }
    } else if (scheduler_algorithm == SRTF) {
        pcb_t *curr = queue->head;
        pcb_t *prev = NULL;
        
        pcb_t *chosen_prev = NULL;
        pcb_t *chosen = queue->head;
        unsigned int min_time_remaining = chosen->total_time_remaining;
        // Finding the right process to dequeue according to time remaining
        while (curr != NULL) {
            if (curr->total_time_remaining < min_time_remaining) {
                chosen_prev = prev;
                chosen = curr;
                min_time_remaining = curr->total_time_remaining;
            }
            prev = curr;
            curr = curr->next;
        }

        if (chosen_prev == NULL) {
            // Dequeueing process at head
            pcb_t *process = queue->head;
            queue->head = queue->head->next;
            if (queue->tail == process) {
                queue->tail = NULL;
            }
            process->next = NULL;
            pthread_mutex_unlock(&queue_mutex);
            return process;
        } else {
            chosen_prev->next = chosen->next;
            if (queue->tail == chosen) {
                queue->tail = chosen_prev;
            }
            chosen->next = NULL;
            pthread_mutex_unlock(&queue_mutex);
            return chosen;
        }
    } else if (scheduler_algorithm == RR) {
        // Round Robin
        pcb_t *process = queue->head;
        queue->head = queue->head->next;
        if (queue->tail == process) {
            queue->tail = NULL;
        }
        process->next = NULL;
        pthread_mutex_unlock(&queue_mutex);
        return process;

    } else {
        // First Come First Serve
        pcb_t *curr = queue->head;
        pcb_t *prev = NULL;
        
        pcb_t *chosen_prev = NULL;
        pcb_t *chosen = queue->head;
        unsigned int earliest_time = chosen->arrival_time;
        // Finding the right process to dequeue according to time remaining
        while (curr != NULL) {
            if (curr->arrival_time < earliest_time) {
                chosen_prev = prev;
                chosen = curr;
                earliest_time = curr->arrival_time;
            }
            prev = curr;
            curr = curr->next;
        }

        if (chosen_prev == NULL) {
            // Dequeueing process at head
            pcb_t *process = queue->head;
            queue->head = queue->head->next;
            if (queue->tail == process) {
                queue->tail = NULL;
            }
            process->next = NULL;
            pthread_mutex_unlock(&queue_mutex);
            return process;
        } else {
            chosen_prev->next = chosen->next;
            if (queue->tail == chosen) {
                queue->tail = chosen_prev;
            }
            chosen->next = NULL;
            pthread_mutex_unlock(&queue_mutex);
            return chosen;
        }
    }
}

/** ------------------------Problem 0-----------------------------------
 * Checkout PDF Section 2 for this problem
 * 
 * is_empty() is a helper function that returns whether the ready queue
 * has any processes in it.
 * 
 * @param queue pointer to the ready queue
 * 
 * @return a boolean value that indicates whether the queue is empty or not
 */
bool is_empty(queue_t *queue)
{
    return (queue->head == NULL);
}

/** ------------------------Problem 1B-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * schedule() is your CPU scheduler.
 * 
 * Remember to specify the timeslice if the scheduling algorithm is Round-Robin
 * 
 * @param cpu_id the target cpu we decide to put our process in
 */
static void schedule(unsigned int cpu_id)
{
    // Extracting FIRST process from Ready Queue
    pcb_t *process = dequeue(rq);

    if (process != NULL) {
        // Setting the process state to running
        process->state = PROCESS_RUNNING;
    }

    // Setting the process to current
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = process;
    pthread_mutex_unlock(&current_mutex);

    // Calling context switch
    context_switch(cpu_id, process, timeslice);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled. This function should block until a process is added
 * to your ready queue.
 *
 * @param cpu_id the cpu that is waiting for process to come in
 */
extern void idle(unsigned int cpu_id)
{
    // Waiting for signal that process is added to ready queue
    pthread_mutex_lock(&queue_mutex);
    while (is_empty(rq)) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    // Scheduling new process
    schedule(cpu_id);
}

/** ------------------------Problem 2 & 3-----------------------------------
 * Checkout Section 4 and 5 for this problem
 * 
 * preempt() is the handler used in Round-robin, Preemptive Priority, and SRTF scheduling.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 * 
 * @param cpu_id the cpu in which we want to preempt process
 */
extern void preempt(unsigned int cpu_id)
{
    // Get the current process
    pthread_mutex_lock(&current_mutex);
    pcb_t *current_process = current[cpu_id];

    // Set the process state to ready
    current_process->state = PROCESS_READY;

    // Add the process back to the ready queue
    enqueue(rq, current_process);

    pthread_mutex_unlock(&current_mutex);

    // Schedule new process
    schedule(cpu_id);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * @param cpu_id the cpu that is yielded by the process
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    // Set the process state to waiting
    pcb_t *current_process = current[cpu_id];
    current_process->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);

    // Schedule new process
    schedule(cpu_id);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3
 * 
 * terminate() is the handler called by the simulator when a process completes.
 * 
 * @param cpu_id the cpu we want to terminate
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    // Set the process state to terminated
    pcb_t *current_process = current[cpu_id];
    current_process->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    
    // Schedule new process
    schedule(cpu_id);
}

/**  ------------------------Problem 1A & 3---------------------------------
 * Checkout PDF Section 3 and 5 for this problem
 * 
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes. 
 * This method will also need to handle priority and SRTF preemption.
 * Look in section 5 of the PDF for more info on priority.
 * Look in section 6 of the PDF for more info on SRTF preemption.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 * 
 * @param process the process that finishes I/O and is ready to run on CPU
 */
extern void wake_up(pcb_t *process)
{
    // Locking the mutex
    pthread_mutex_lock(&current_mutex);

    // Set the process state to ready
    process->state = PROCESS_READY;

    // Add the process to the ready queue
    enqueue(rq, process);

    // Preempting if new process has less time remaining to complete
    if (scheduler_algorithm == SRTF) {
        // Shortest Remaining Time First
        unsigned int most_time_remaining = 0;
        unsigned int victim_cpu = 0;

        for (unsigned int i = 0; i < cpu_count; i++) {
            pcb_t *current_process = current[i];
            
            if (current[i] != NULL) {
                // Preempting if new process has less time remaining
                if (most_time_remaining < current_process->total_time_remaining) {
                    most_time_remaining = current_process->total_time_remaining;
                    victim_cpu = i;
                }
            } else {
                pthread_mutex_unlock(&current_mutex);
                return;
            }
        }
        // Unlocking the mutex
        pthread_mutex_unlock(&current_mutex);

        if (most_time_remaining != 0) {
            if (most_time_remaining > process->total_time_remaining) {
                force_preempt(victim_cpu);
                return;
            }
            else {
                return;
            }
        } else {
            return;
        }
    // Preempting if new process has higher priority (lower functional priority)
    } else if (scheduler_algorithm == PA) {
        // Preemptive Priority with Aging
        double new_process_priority = priority_with_age(get_current_time(), process);

        double lowest_priority = 0;
        unsigned int lowest_priority_cpu = 0;

        for (unsigned int i = 0; i < cpu_count; i++) {
            pcb_t *current_process = current[i];
            
            if (current[i] != NULL) {
                double current_process_priority = priority_with_age(get_current_time(), current_process);
                // Preempting if new process has higher priority (lower functional priority)
                
                if (lowest_priority < current_process_priority) {
                    lowest_priority = current_process_priority;
                    lowest_priority_cpu = i;
                }
            } else {
                pthread_mutex_unlock(&current_mutex);
                return;
            }
        }
        // Unlocking the mutex
        pthread_mutex_unlock(&current_mutex);

        if (lowest_priority != 0) {
            if (new_process_priority < lowest_priority) {
                force_preempt(lowest_priority_cpu);
                return;
            } else {
                return;
            }
        } else {
            return;
        }
    }
    pthread_mutex_unlock(&current_mutex);
}

/**
 * main() simply parses command line arguments, then calls start_simulator().
 * Add support for -r, -p, and -s parameters. If no argument has been supplied, 
 * you should default to FCFS.
 * 
 * HINT:
 * Use the scheduler_algorithm variable (see student.h) in your scheduler to 
 * keep track of the scheduling algorithm you're using.
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
                        "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p <age weight> | -s ]\n"
                        "    Default : FCFS Scheduler\n"
                        "         -r : Round-Robin Scheduler\n1\n"
                        "         -p : Priority Aging Scheduler\n"
                        "         -s : Shortest Remaining Time First\n");
        return -1;
    }

    // Default to FCFS
    scheduler_algorithm = FCFS;
    age_weight = 0;

    /* Parse the command line arguments */
    cpu_count = strtoul(argv[1], NULL, 0);

    // Command Line code
    if (argc > 2) {
        if (strcmp(argv[2], "-r") == 0) {
            scheduler_algorithm = RR;
            timeslice = atoi(argv[3]);
        } else if (strcmp(argv[2], "-p") == 0) {
            scheduler_algorithm = PA;
            age_weight = (unsigned int) atoi(argv[3]);
        } else if (strcmp(argv[2], "-s") == 0) {
            scheduler_algorithm = SRTF;
            age_weight = 0;
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t *) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    rq = (queue_t *)malloc(sizeof(queue_t));
    assert(rq != NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}

#pragma GCC diagnostic pop
