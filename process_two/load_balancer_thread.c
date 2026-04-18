#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "../shared.h"

void* load_balancer_thread(void *arg){
    printf("[balancer] thread started, highest priority\n");
    while(1){
        sem_wait(&balancer_signal);//thread is blocked and wakes up when called
        printf("\n[balancer] got signal, deciding what to do...\n");//wakeup action
        pthread_mutex_lock(&grid_mutex);//shared grid data is locked
        if(cascade_detected){
            printf("[balancer] cascade situation, isolating overloaded regions\n");
            int i;
            for(i = 0; i < MAX_REGIONS; i++){
                if(grid_loads[i] > BASE_CAPACITY) {
                    grid_loads[i] = 0;
                    grid_status[i] = STATUS_BLACKOUT;
                    printf("[balancer] region %d has been isolated\n", i);
            }
        }
    }
    else if(overload_region >= 0){
        int region = overload_region;
        int excess = grid_loads[region] - BASE_CAPACITY;
        printf("[balancer] region %d is overloaded by %d kW\n", region, excess);
         if(excess > MAX_TRANSFER){
            printf("[balancer] excess too high to transfer, isolating region %d\n", region);
            grid_loads[region] = 0;
            grid_status[region] = STATUS_BLACKOUT;
         }
          else{
            int region1 = (region + 1) % MAX_REGIONS;//calculates the next region after one step
            int region2 = (region + 2) % MAX_REGIONS;//calculates the next region after two steps
            int total_capacity = (BASE_CAPACITY - grid_loads[region1]) + (BASE_CAPACITY - grid_loads[region2]);
            if(total_capacity >= excess){
                 printf("[balancer] redistributing %d kW from region %d to %d and %d\n",excess, region, region1, region2);
                 grid_status[region] = STATUS_REDISTRIBUTING;
                 grid_loads[region] = BASE_CAPACITY - 10;// keeping 10 as a safety buffer with the region that is distributing its extra power
                 grid_loads[region1] += excess / 2;//Adds half the excess load to region1.
                 grid_loads[region2] += excess / 2;//Adds the other half to region2
            }
            else{
                  printf("[balancer] not enough capacity in other regions, isolating region %d\n", region);
                  grid_loads[region] = 0;//isolates the region as no other option is left
                  grid_status[region] = STATUS_BLACKOUT;
            }
          }
        }
        print_grid_status();
        pthread_mutex_unlock(&grid_mutex);//releases the lock so others can acess the grid data again
        sleep(1);
    }
 return NULL;
}

