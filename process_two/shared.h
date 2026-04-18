#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <semaphore.h>

#define MAX_REGIONS 3
#define BASE_CAPACITY 300
#define MAX_TRANSFER 30

#define STATUS_STABLE 0
#define STATUS_OVERLOAD 1
#define STATUS_REDISTRIBUTING 2
#define STATUS_BLACKOUT 3

// Global state (extern declarations)
extern int grid_loads[MAX_REGIONS];
extern int grid_status[MAX_REGIONS];
extern pthread_mutex_t grid_mutex;
extern sem_t monitor_signal;
extern sem_t balancer_signal;
extern int cascade_detected;
extern int overload_region;
extern int chid;
extern int client_coid;

// Message structures
struct load_command {
    int region_id;
    int new_load;
};

struct server_reply {
    int status;
    char message[100];
};

struct grid_status {
    int region_loads[MAX_REGIONS];
    int region_status[MAX_REGIONS];
    char status_message[200];
};

// Function declarations
void print_grid_status(void);
void* monitor_region_thread(void *arg);
void* comparator_thread(void *arg);
void* load_balancer_thread(void *arg);
void* message_handler_thread(void *arg);
void* status_broadcaster_thread(void *arg);
void* cli_thread(void *arg);

#endif
