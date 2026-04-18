#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../shared.h"

void* monitor_region_thread(void *arg)
{
    int region = *(int*)arg;
    printf("[monitor %d] started\n", region);
        while(1) {
            pthread_mutex_lock(&grid_mutex);//grid data ius locked before reading
            int current_load = grid_loads[region];//reads current load for specific region
            pthread_mutex_unlock(&grid_mutex);//releases lock after reading
            if(current_load > BASE_CAPACITY) {
                printf("[monitor %d] overload, current load is %d kW\n", region, current_load);
                sem_post(&monitor_signal);//tells the semaphore to wake up the counter thread
            }
            sleep(1);//waiits for 1sec before checking again
        }
    return NULL;
}
