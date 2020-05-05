#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "csapp.h"

static void terminate(int status);

void SIGHUP_handler() {
    terminate(EXIT_SUCCESS);
}

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int option;
    while((option = getopt(argc, argv, "p:")) != EOF) {
        switch(option){
            case 'p':
                if(argc != 3) { //if not in format '-p <port>'
                    debug("main.c: invalid arguments");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                debug("main.c: invalid arguments");
                exit(EXIT_FAILURE);
        }
    }

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    //setting up SIGHUP handler
    struct sigaction new_SIGHUP;
    new_SIGHUP.sa_handler = SIGHUP_handler;
    sigemptyset(&new_SIGHUP.sa_mask);
    new_SIGHUP.sa_flags = 0;
    sigaction(SIGHUP, &new_SIGHUP, NULL);

    //setting up server socket & loop
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    listenfd = Open_listenfd(argv[2]);
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd,(SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, pbx_client_service, connfdp);
    }
    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}
