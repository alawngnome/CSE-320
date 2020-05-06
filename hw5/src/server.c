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

void *pbx_client_service(void *arg) {
    //saving the arg & free
    int arg_FD = *(int *)arg;
    Free(arg);

    signal(SIGPIPE, SIG_IGN);

    //detach thread
    Pthread_detach(pthread_self());
    //register client FD
    TU *new_TU  = pbx_register(pbx, arg_FD);
    //enter service loop
    FILE *arg_filestream = fdopen(arg_FD, "r");
    while(1){
        //read message
        char *message = malloc(1); //complete command to be read
        int counter = 1;
        int c; //character to be read
        while((c = fgetc(arg_filestream)) != EOF) {
            if(c == '\r') {
                if((c = fgetc(arg_filestream)) == '\n') {
                    message[counter - 1] = '\0';
                    break;
                }
            }
            message[counter - 1] = c;
            message = realloc(message, (++counter)*sizeof(char)); //realloc to +1 size
        }
        if(c == EOF) {
            //free message and uregister
            debug("server.c: EOF detected and unregistering started");
            debug("server.c: hangup called");
            tu_hangup(new_TU);
            debug("server.c:hangup finished executing");
            fclose(arg_filestream);
            free(message);
            debug("server.c: unregister called");
            pbx_unregister(pbx, new_TU);
            return NULL;
        }

        // size_t n;
        // rio_t rio;
        // Rio_readinitb(&rio, arg_FD);
        // char message[MAXLINE]; //A PRIORI!!!!!!
        // while((n = Rio_readlineb(&rio, message, MAXLINE)) != 0) {
        //     debug("server received %d bytes", (int)n);
        //     break;
        // }
        // if(n == 0) {
        //     debug("deregistering");
        //     pbx_unregister(pbx, new_TU);
        //     return NULL;
        // }

        debug("message_length is %ld", strlen(message));
        debug("message is %s", message);

        int message_length = strlen(message);
        //determine what message it is
        if(message[0] == 'p') {
            //pickup
            if((strstr(message, "pickup") != NULL) && message_length == 6) {
                debug("pickup called in server.c");
                tu_pickup(new_TU);
            }
        }
        else if(message[0] == 'h') {
            //hangup
            if((strstr(message, "hangup") != NULL) && message_length == 6) {
                tu_hangup(new_TU);
            }
        }
        else if(message[0] == 'd') {
            //dial #
            if(strstr(message, "dial") != 0) {
                tu_dial(new_TU, atoi(&message[5]));
            }
        }
        else if(message[0] == 'c') {
            //chat ...arbitrary text...
            if(strstr(message, "chat") != NULL) {
                tu_chat(new_TU, &message[5]);
            }
        } else { //if message does not match any of the above
            debug("server.c: message not understood");
        }
        free(message);
    }
    return NULL;
}