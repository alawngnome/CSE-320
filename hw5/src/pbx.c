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

sem_t mutex; //mutex

//data about the tu struct
struct tu {
    int fileno; //file descriptor
    volatile int state; //state of telephone unit
    int peer_tu_fileno; //fileno of a connected TU
};
//data about the pbx struct
struct pbx {
    struct tu* TU_list[PBX_MAX_EXTENSIONS]; //indices = extensions, data = pointer to TU
};

PBX *pbx_init(){
    pbx = malloc(sizeof(struct pbx)); //initialize the global pbx object
    Sem_init(&mutex, 0, 1); //intialize semaphore
    return pbx;
}

void pbx_shutdown(PBX *pbx){
    //find all clients and send shutdown signal to each one
    for(int i = 0; i < PBX_MAX_EXTENSIONS; i++) {
        if(pbx->TU_list[i] != NULL) {
            shutdown(pbx->TU_list[i]->fileno, SHUT_RDWR);
        }
    }
    while(1) {
        int full_lap = 1;
        for(int i = 0; i < PBX_MAX_EXTENSIONS; i++) {
            if(pbx->TU_list[i] != NULL) {
                full_lap = 0;
            }
        }
        if(full_lap == 1) {
            break;
        }
    }
    free(pbx);
}

TU *pbx_register(PBX *pbx, int fd){
    P(&mutex);
    TU *new_TU = malloc(sizeof(struct tu));
    if(new_TU == NULL) { //if registration failed
        return NULL;
    }
    //initializing the TU
    new_TU->fileno = fd;
    new_TU->state = TU_ON_HOOK;
    pbx->TU_list[fd] = new_TU; //putting it in the pbx list
    //writing ON HOOK # to client
    char client_message[MAXLINE];
    sprintf(client_message, "ON HOOK %d\r\n", new_TU->fileno);
    Rio_writen(fd, client_message, strlen(client_message));
    //returning TU
    V(&mutex);
    return new_TU;
}

int pbx_unregister(PBX *pbx, TU *tu){
    //if already removed from TU_list
    if(pbx->TU_list[tu->fileno] == NULL)
        return -1;
    pbx->TU_list[tu->fileno] = NULL; //remove from pbx's TU list
    free(tu); //free
    return 0; //if registration succeeds
}

int tu_fileno(TU *tu){
    if(tu == NULL) //if tu does not exist
        return -1;
    return tu->fileno;
}

int tu_extension(TU *tu){
    if(tu == NULL) //if tu does not exist
        return -1;
    return tu->fileno;
}

int tu_pickup(TU *tu){
    debug("pickup being executed start");
    P(&mutex);
    debug("pickup being executed");
    if(tu == NULL){
        debug("tu ended up being null");
        return -1;
    }
    char client_message[MAXLINE];
    if(tu->state == TU_ON_HOOK){ //currently ON HOOK
        tu->state = TU_DIAL_TONE;
        sprintf(client_message, "DIAL TONE\r\n");
    }else if(tu->state == TU_RINGING) { //currently RINGING
        //set tu to CONNECTED
        tu->state = TU_CONNECTED;
        sprintf(client_message, "CONNECTED %d\r\n", tu_extension(tu));
        //set peer tu to CONNECTED
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_CONNECTED;
        //write confirmation messages
        Rio_writen(tu->fileno, client_message, strlen(client_message));
        Rio_writen(tu->peer_tu_fileno, client_message, strlen(client_message));
        V(&mutex);
        return 0;
    } else { //currently any other state
        if(tu->state == TU_DIAL_TONE) { sprintf(client_message, "DIAL TONE\r\n");
        } else if(tu->state == TU_RING_BACK) { sprintf(client_message, "RING BACK\r\n");
        } else if(tu->state == TU_BUSY_SIGNAL){ sprintf(client_message, "BUSY SIGNAL\r\n");
        } else if(tu->state == TU_CONNECTED) { sprintf(client_message, "CONNECTED %d\r\n", tu_extension(tu));
        } else if(tu->state == TU_ERROR) { sprintf(client_message, "ERROR\r\n"); }
    }
    Rio_writen(tu->fileno, client_message, strlen(client_message));
    V(&mutex);
    return 0;
}

int tu_hangup(TU *tu){
    P(&mutex);
    if(tu == NULL)
        return -1;
    char client_message[MAXLINE];
    if(tu->state == TU_CONNECTED){ //if CONNECTED
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        //set peer tu to DIAL TONE
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_DIAL_TONE;
        //write confirmation messages
        sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
        Rio_writen(tu->fileno, client_message, strlen(client_message));
        sprintf(client_message, "DIAL TONE\r\n");
        Rio_writen(tu->peer_tu_fileno, client_message, strlen(client_message));
        V(&mutex);
        return 0;
    }else if(tu->state == TU_RING_BACK){ //if RING BACK
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        //set peer tu to ON HOOK
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_ON_HOOK;
        //send confirmation messages
        sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
        Rio_writen(tu->fileno, client_message, strlen(client_message));
        sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
        Rio_writen(tu->peer_tu_fileno, client_message, strlen(client_message));
        V(&mutex);
        return 0;
    }else if(tu->state == TU_RINGING){ //if RINGING
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        //set peer tu to DIAL TONE
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_DIAL_TONE;
        //write confirmation messages
        sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
        Rio_writen(tu->fileno, client_message, strlen(client_message));
        sprintf(client_message, "DIAL TONE\r\n");
        Rio_writen(tu->peer_tu_fileno, client_message, strlen(client_message));
        V(&mutex);
        return 0;
    }else if(tu->state == TU_DIAL_TONE || tu->state == TU_BUSY_SIGNAL || tu->state == TU_ERROR) {
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
    }else if(tu->state == TU_ON_HOOK) { //if nothing else, keep current state
        sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
    }

    Rio_writen(tu->fileno, client_message, strlen(client_message));
    V(&mutex);
    return 0;
}

int tu_dial(TU *tu, int ext){
    P(&mutex);
    if(tu == NULL || ext < 0)
        return -1;
    char client_message[MAXLINE];
    //check that extension number is registered
    if(pbx->TU_list[ext] == NULL) {
        //set state to be ERROR
        tu->state = TU_ERROR;
        sprintf(client_message, "ERROR\r\n");
    }else if(tu->state == TU_DIAL_TONE) { //if DIAL TONE
        if(pbx->TU_list[ext]->state == TU_ON_HOOK) { //if peer = DIAL TONE
            tu->peer_tu_fileno = ext; //set peer fileno
            pbx->TU_list[ext]->peer_tu_fileno = tu_fileno(tu); //set peer's peer fileno
            //set dialing tu(tu) to RING BACK
            tu->state = TU_RING_BACK;
            //set dialed tu(peer tu) to RINGING
            pbx->TU_list[ext]->state = TU_RINGING;
            debug("pbx->TU_list[tu->peer_tu_fileno]->state = %d", pbx->TU_list[tu->peer_tu_fileno]->state);
            //write confirmation messages
            sprintf(client_message, "RING BACK\r\n");
            Rio_writen(tu->fileno, client_message, strlen(client_message));
            sprintf(client_message, "RINGING\r\n");
            Rio_writen(pbx->TU_list[tu->peer_tu_fileno]->fileno, client_message, strlen(client_message));
            V(&mutex);
            return 0;
        }else { //if peer != DIAL TONE
            tu->state = TU_BUSY_SIGNAL;
            sprintf(client_message, "BUSY SIGNAL\r\n");
        }
    } else { //if not DIAL TONE
        //leave default state alone
        if(tu->state == TU_ON_HOOK){ sprintf(client_message, "ON HOOK %d\r\n", tu->fileno);
        }else if(tu->state == TU_RINGING) { sprintf(client_message, "RINGING\r\n");
        }else if(tu->state == TU_RING_BACK) { sprintf(client_message, "RING BACK\r\n");
        }else if(tu->state == TU_BUSY_SIGNAL) { sprintf(client_message, "BUSY SIGNAL\r\n");
        }else if(tu->state == TU_CONNECTED) { sprintf(client_message, "CONNECTED %d\r\n", tu_extension(tu));
        }else if(tu->state == TU_ERROR) { sprintf(client_message, "ERROR\r\n"); }
    }

    Rio_writen(tu->fileno, client_message, strlen(client_message));
    V(&mutex);
    return 0;
}

int tu_chat(TU *tu, char *msg){
    if(tu->state != TU_CONNECTED)
        return -1;
    debug("%s", msg);
    char *appended_msg = strcat(msg, "\r\n\0");
    Rio_writen(tu->peer_tu_fileno, appended_msg, strlen(appended_msg));
    return 0;
}