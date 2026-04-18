#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "../shared.h"
void* comparator_thread(void *arg){
    printf("[Comparator] thread started\n");
    while(1){                              //runs forever
        sem_wait(&monitor_signal);         //blocks here until cli calls sem_post(&monitor_signal)
        printf("[Comparator] checking regions now.....\n");
        pthread_mutex_lock(&grid_mutex);   //locks shared data before reading gird_loads
        int overload_count=0;
        int i;
        for(i=0;i<MAX_REGIONS;i++){        //loops thru all regions, if any load exceed Base_capacity then increments count
            if(grid_loads[i]>BASE_CAPACITY)
            	overload_count++;              //counts how many overloaded regions were found
        }
        printf("[Comparator] found %d overload region\n", overload_count);
        if(overload_count>=2){
            printf("[Comparator] cascade detected, signaling balancer\n");
            cascade_detected=1;             // if 2 or more regions overloaded
        }
        else{
            cascade_detected=0;
            for(i=0;i<MAX_REGIONS;i++){      //finds which single region is overloaded and stores its index in overload_region
                if(grid_loads[i]>BASE_CAPACITY){
                    overload_region=i;
                    break;
                }
            }
        }
        sem_post(&balancer_signal);         //wakes balancer
        pthread_mutex_unlock(&grid_mutex); //releases mutex
    }
return NULL;                              //null for void*
}
