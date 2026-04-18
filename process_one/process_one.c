#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <pthread.h>
struct load_command { //command to whcih region and what load
    int region_id;
    int new_load;
};
struct server_reply { //what does server send back after processing a command
    int status;
    char message[100];
};
struct grid_status { //broadcasts
    int region_loads[3];
    int region_status[3];
    char status_message[200];
};
int server_coid = -1; //cnnection id is -1=not connected to server yet
int chid = -1;        //process's channel id is not created yet
void* status_receiver_thread(void *arg) { //thread fn to listem to incoming status broadcasts from the server
    struct grid_status status; //a struct to hold the incoming data
    int rcvid; //to store value of msgreceive (+ve means real message came)
    while (1) {
        rcvid = MsgReceive(chid, &status, sizeof(status), NULL); //bloakcs and waits for servers braodcaster thread to send status update
        if (rcvid > 0) { //if it was actual message and not some pulse
            printf("\n Incoming Grid Update \n");
            printf("R0: %d kW | R1: %d kW | R2: %d kW\n",status.region_loads[0], status.region_loads[1], status.region_loads[2]);
            printf("Message: %s\n", status.status_message); //message string the server included in broadcast
            printf("\n\n");
        }
    }
    return NULL;
}
void* cli_thread(void *arg) { //thread to handle user input fro, terminal
    char command[100]; //buffer
    while (1) {
        printf("GridControl> ");
        fflush(stdout); //forces previous print line to appear
        if (fgets(command, sizeof(command), stdin) == NULL) break; //reads input; if null, it breaks
        if (strncmp(command, "load", 4) == 0) {
            int region, kw;
            if (sscanf(command, "load %d %d", &region, &kw) == 2 && region >= 0 && region <= 2 && kw >= 0 && kw <= 500) { // makes sure input was in range of 0-2 load and 0-500kw
                    struct load_command cmd = {region, kw}; //creates command struct with values to send to server
                    struct server_reply reply; //declares the reply struct that the server will fill in when it responds
                    if (MsgSend(server_coid, &cmd, sizeof(cmd), &reply, sizeof(reply)) >= 0) { //sends command to the server and blocks waiting for reply. rely struct gets filled automatically when server calls MsgReply
                        printf("Response: %s\n", reply.message);
                }
                else {
                    printf("Communication error: Could not reach server.\n");
                }
            }
            else {
                printf("Usage: load <0-2> <0-500>\n");
            }
        }
        else if (strncmp(command, "exit", 4) == 0) {
            printf("Exiting system...\n");
            exit(0);
        }
        else if (strlen(command) > 1) {
            printf("Commands: load <id> <kw>, exit\n"); // if command not recognized, prints available comamnds
        }
    }
    return NULL;
}
int main() {
    int server_pid, server_chid;
    printf("Main Substation Initializing...\n");
    printf("Enter Server PID: "); //server pid from server terminal
    scanf("%d", &server_pid);
    printf("Enter Server Channel ID: ");
    scanf("%d", &server_chid); //also on server teminal
    getchar(); // Clear newline from buffer
    chid = ChannelCreate(0);
    if (chid < 0) {
        perror("Failed to create local channel");
        return -1;
    }
    server_coid = ConnectAttach(0, server_pid, server_chid, 0, 0); //connect to servers channels using pid and channel id. returns a conneciton id that is used in msgsend calls. first 0 is node(0 is local machine and last 2 0's are flags)
    if (server_coid < 0) {
        perror("Connection to server failed");
        return -1;
    }
    printf("Link established with PID %d. Starting monitoring threads...\n\n", server_pid);
    pthread_t rcv_thread, cli_tid; //thread handles for 2 threads
    pthread_create(&rcv_thread, NULL, status_receiver_thread, NULL); //starts status receiver thread with default attributes.
    pthread_create(&cli_tid, NULL, cli_thread, NULL); // starts cli thread.
    pthread_join(cli_tid, NULL); //main thread waits for cli thread to finish. cli thread closes ONLY if fgets NULL. Receiver thread is killed when process exits.
    ConnectDetach(server_coid); //cleanly disconnects from server's channel before exiting
    ChannelDestroy(chid);//destroys client's own channel before exit
    return 0;
}
