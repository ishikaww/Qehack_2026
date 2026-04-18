#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include "../shared.h"
void* status_broadcaster_thread(void *arg)
{
    printf("[broadcaster] started, waiting for client to connect\n\n");
    while(1) {
        if(client_coid > 0) {//checks if a client is connected
            pthread_mutex_lock(&grid_mutex);//locks grid data before reading
            struct grid_status status;//declaring loacal struct to hold snapshot of the grid
            int i;
            for(i = 0; i < MAX_REGIONS; i++) {
                status.region_loads[i] = grid_loads[i];//copies current load value
                status.region_status[i] = grid_status[i];//copies status whether stable or not
            }
            sprintf(status.status_message, "grid status update");//reads the string when msg is received
            pthread_mutex_unlock(&grid_mutex);//releases the lock after copying
            if(MsgSend(client_coid, &status, sizeof(status), NULL, 0) < 0) {
                printf("[broadcaster] client disconnected\n");
                client_coid = -1;//resets the connection id
            }
        }
        sleep(2);//waits for 2sec before repeating the loop
    }
    return NULL;
}
