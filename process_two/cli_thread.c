#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "../shared.h"

void* cli_thread(void *arg) //defines cli thread function
{
    printf("[CLI] thread started, priority 10\n\n");
    while (1) {  //infinite loop(cli runs until program ends)
        printf("-- Commands --\n");
        printf("  load <region 0-2> <kW>   example: load 0 320\n");
        printf("  status\n");
        printf("  reset\n");
        printf("  test1 / test2 / test3\n");
        printf("\n> ");
        fflush(stdout); //forces ">"to appear immediately"

        char command[100]; //100 character buffer
        if (fgets(command, sizeof(command), stdin) == NULL) //breaks if size of the command is 0
            break;

        if (strncmp(command, "load", 4) == 0) { //checks of first 4 char of input are "load"
            int region, kw;
            if (sscanf(command, "load %d %d", &region, &kw) == 2 && //giving range to code
                region >= 0 && region <= 2 &&
                kw >= 0 && kw <= 500)
            {
                pthread_mutex_lock(&grid_mutex); //locks mutex before touching shred data to prevent race condition
                grid_loads[region] = kw;
                grid_status[region] = STATUS_STABLE;
                pthread_mutex_unlock(&grid_mutex); //unlocks mutex so other threads can access shared data again
                printf("ok, region %d is now %d kW\n\n", region, kw);
                sem_post(&monitor_signal); //signals monitor thread and increments semaphore
            }
            else {
                printf("Bad input. Region must be 0-2, load must be 0-500\n\n");
            }
        }
        else if (strncmp(command, "status", 6) == 0) { //checks if command stards with "status"
            pthread_mutex_lock(&grid_mutex); //locks mutex
            print_grid_status(); //prints status
            pthread_mutex_unlock(&grid_mutex); //finally unlocks
        }
        else if (strncmp(command, "reset", 5) == 0) { //reset to default values
            pthread_mutex_lock(&grid_mutex);
            grid_loads[0] = 250;
            grid_loads[1] = 280;
            grid_loads[2] = 260;
            grid_status[0] = STATUS_STABLE;
            grid_status[1] = STATUS_STABLE;
            grid_status[2] = STATUS_STABLE;
            pthread_mutex_unlock(&grid_mutex);
            printf("reset done, loads back to 250/280/260\n\n");
        }
        else if (strncmp(command, "test1", 5) == 0) { //checks if command is "test1"
            // single overload under 30kW threshold, should redistribute
            pthread_mutex_lock(&grid_mutex);
            grid_loads[0] = 320; //sets region 0 to 320 kw under mutex protection
            pthread_mutex_unlock(&grid_mutex);
            printf("test1: set region 0 to 320 kW (mild overload)\n\n");
            sem_post(&monitor_signal);
        }
        else if (strncmp(command, "test2", 5) == 0) {
            // overload exceeds max transfer, should isolate
            pthread_mutex_lock(&grid_mutex);
            grid_loads[0] = 380;
            pthread_mutex_unlock(&grid_mutex);
            printf("test2: set region 0 to 380 kW (severe overload)\n\n");
            sem_post(&monitor_signal);
        }
        else if (strncmp(command, "test3", 5) == 0) {
            // two regions overloaded, cascade scenario
            pthread_mutex_lock(&grid_mutex);
            grid_loads[0] = 350;
            grid_loads[1] = 350;
            pthread_mutex_unlock(&grid_mutex);
            printf("test3: set region 0 and 1 to 350 kW each (cascade)\n\n");
            sem_post(&monitor_signal);
        }
        else {
            printf("unknown command\n\n");
        }
    }

    return NULL;
}
