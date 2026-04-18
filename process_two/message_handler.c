#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/neutrino.h>
#include <time.h>
#include "../shared.h"

void* message_handler_thread(void *arg)
{
    printf("[msg handler] started, waiting for commands\n\n");//prints when thread launches to see its active
    struct load_command cmd;
    struct server_reply reply;
    int rcvid;
    struct timespec t_start, t_end;
    static long min_latency = 999999;
    static long max_latency = 0;
    while(1) {
        rcvid = MsgReceive(chid, &cmd, sizeof(cmd), NULL);//thread is blocked until a msg is received, once received, the thread becomes active
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        if(rcvid < 0) { // if msg is negative, something is wrong
            printf("[msg handler] something went wrong receiving message\n");
            continue;
        }
        if(client_coid < 0) { // checks for the previous client
            client_coid = rcvid;//Saves the sender's ID as the client connection ID, this is used to send updates back to client
            printf("[msg handler] client connected\n");
        }
        printf("\n[msg handler] got command - region %d, load %d kW\n",cmd.region_id, cmd.new_load);
        if(cmd.region_id >= 0 && cmd.region_id < MAX_REGIONS && cmd.new_load >= 0 && cmd.new_load <= 500){
            pthread_mutex_lock(&grid_mutex);// locks the grid data
            int old_load = grid_loads[cmd.region_id];//saves the current load before changing
            grid_loads[cmd.region_id] = cmd.new_load;//sets the load to new value
            grid_status[cmd.region_id] = STATUS_STABLE;//resets the regions status to stable
            printf("[msg handler] region %d changed from %d to %d kW\n", cmd.region_id, old_load, cmd.new_load);
            pthread_mutex_unlock(&grid_mutex);//releases the lock
            sem_post(&monitor_signal);//comparator is waiting on this semaphore and wakes up to check if new load causes overload
            reply.status = 1;// sets 1 on success
            sprintf(reply.message, "region %d updated to %d kW", cmd.region_id, cmd.new_load);
        }
        else {
            reply.status = 0;// 0 indicates failure
            sprintf(reply.message, "invalid region or load value");
            printf("[msg handler] got bad input, ignoring\n");
        }

    MsgReply(rcvid, 0, &reply, sizeof(reply));//sends reply using sender ID saved in rcvid
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    long latency_us = (t_end.tv_sec - t_start.tv_sec) * 1000000L +
                      (t_end.tv_nsec - t_start.tv_nsec) / 1000;
    if(latency_us < min_latency) min_latency = latency_us;
    if(latency_us > max_latency) max_latency = latency_us;
    printf("[msg handler] latency: %ld us  (min: %ld us  max: %ld us)\n",
            latency_us, min_latency, max_latency);
    }
return NULL;
}
