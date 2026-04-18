#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/neutrino.h> //ipc functionalities
#include "../shared.h"

int grid_loads[MAX_REGIONS]={250,280,260}; //array storing 3 loads
int grid_status[MAX_REGIONS]= {STATUS_STABLE, STATUS_STABLE, STATUS_STABLE}; // all start as stable
pthread_mutex_t grid_mutex = PTHREAD_MUTEX_INITIALIZER; //shortcut to declare and call a mutex
sem_t monitor_signal; //semaphore used by cli to wake up comparator when load changes
sem_t balancer_signal; //semaphore used by comparator to wake up balancer thread
int cascade_detected = 0; //flag is 1 if >2 regions overloaded
int overload_region = -1; //index of overloaded region
int chid; //creating channel ID for QNX message passing
int client_coid = -1; //connection id for client. -1=no client yet
void print_grid_status()
{
    int i;
    printf("\ncurrent grid state:\n");
    for(i=0;i<MAX_REGIONS;i++){
        char *s= "Stable";
        if(grid_status[i]==STATUS_OVERLOAD) s = "overload";
        else if(grid_status[i] == STATUS_REDISTRIBUTING) s = "redistributing";
        else if(grid_status[i] == STATUS_BLACKOUT) s = "blackout";
        printf("  region %d: %d kW (%s)\n", i, grid_loads[i], s);
    }
    printf("\n");
}

int main(){
    printf("Server Starting, PID: %d\n\n", getpid()); //useful to connect process 1
    chid= ChannelCreate(0); //o means no special flags, channel id stored in chid is returned
    if(chid<0){
        printf("Error: Could not create channel\n");
        return -1;
    }
    sem_init(&monitor_signal,0,0); //(&monitor_signal,0,0)=(semaphore pointer,sem shared between two threads of same process, sem is initially locked)
    sem_init(&balancer_signal,0,0); //same but for balanmcer_signal
    pthread_t tids[8]; //array for 8 thread ids
    pthread_attr_t attr; //attributes like priority etc
    struct sched_param param; //sets scheduling parameter for a process
    int region_ids[3] = {0, 1, 2};
    int i;

    void make_thread(pthread_t *t, void*(*fn)(void*), void *arg, int prio) {
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, SCHED_RR); //scheduling policy is round robin
        param.sched_priority = prio; //sets priority in param struct to whatever was enetered
        pthread_attr_setschedparam(&attr, &param); //applies param struct into attr object
        pthread_create(t, &attr, fn, arg); //creates thread wiht 4 arguments (t-where to store new thread id,&attr — the configured attributes (Round Robin + your priority))

}
make_thread(&tids[7], status_broadcaster_thread, NULL, 9); //lowest priority, broadcasts status periodically
make_thread(&tids[6], message_handler_thread, NULL, 11); //NULL is used because thread doesn't need any starting info to do its job

for (i=0;i<3;i++){
    make_thread(&tids[i],monitor_region_thread, &region_ids[i],12);
}
make_thread(&tids[3],comparator_thread,NULL,13);
make_thread(&tids[4],load_balancer_thread,NULL,14);
make_thread(&tids[5],cli_thread,NULL,10);
printf("all 8 threads running\n\n");
for(i = 0; i < 8; i++)
    pthread_join(tids[i], NULL); //waits for all 8 threads to finish becfore continuing

ChannelDestroy(chid); //cleaning up ipc
return 0;
}
