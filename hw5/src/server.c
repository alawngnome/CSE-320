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
    //detach thread
    Pthread_detach(pthread_self());
    //register client FD
    TU *new_TU  = pbx_register(pbx, arg_FD);
    //enter service loop
    while(1){
        //read message
        size_t n;
        rio_t rio;
        Rio_readinitb(&rio, arg_FD);
        char message[MAXLINE]; //A PRIORI!!!!!!
        while((n = Rio_readlineb(&rio, message, MAXLINE)) != 0) {
            debug("server received %d bytes", (int)n);
            break;
        }
        int message_length = strlen(message);
        //determine what message it is
        if(message[0] == 'p') {
            //pickup
            tu_pickup(new_TU);
        }
        else if(message[0] == 'h') {
            //hangup
            tu_hangup(new_TU);
        }
        else if(message[0] == 'd') {
            //dial #
            ;char dial_char[message_length - 5];
            memcpy(dial_char, &message[5], message_length - 5);
            tu_dial(new_TU, atoi(dial_char));
        }
        else if(message[0] == 'c') {
            //chat ...arbitrary text...
            ;char chat_char[message_length - 5];
            memcpy(chat_char, &message[5], message_length - 5);
            tu_chat(new_TU, chat_char);
        } else { //if message does not match any of the above
            debug("server.c: message not understood");
            continue;
        }
    }
    return NULL;
}