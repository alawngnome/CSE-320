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
    for(int i = 0; i < PBX_MAX_EXTENSIONS; i++) {
        pbx->TU_list[i] = NULL;
    }
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
    //loop until all clients are removed from the list
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
    dprintf(fd, "ON HOOK %d\r\n", new_TU->fileno);
    //returning TU
    V(&mutex);
    return new_TU;
}

int pbx_unregister(PBX *pbx, TU *tu){
    P(&mutex);
    //if already removed from TU_list
    if(pbx->TU_list[tu->fileno] == NULL){
        debug("unregister: error detected");
        return -1;
    }
    pbx->TU_list[tu->fileno] = NULL; //remove from pbx's TU list
    //close(tu->fileno); why am i double closing
    free(tu); //free TU
    debug("unregister: tu successfully freed and unregister complete");
    V(&mutex);
    return 0; //if unregistration succeeds
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
    P(&mutex);
    if(tu == NULL){
        V(&mutex);
        return -1;
    }
    if(tu->state == TU_ON_HOOK){ //currently ON HOOK
        tu->state = TU_DIAL_TONE;
        dprintf(tu->fileno, "DIAL TONE\r\n");
    }else if(tu->state == TU_RINGING) { //currently RINGING
        //set tu to CONNECTED
        tu->state = TU_CONNECTED;
        //set peer tu to CONNECTED
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_CONNECTED;
        //write confirmation messages
        dprintf(tu->fileno, "CONNECTED %d\r\n", tu->peer_tu_fileno);
        dprintf(tu->peer_tu_fileno, "CONNECTED %d\r\n", tu->fileno);
        V(&mutex);
        return 0;
    } else { //currently any other state
        if(tu->state == TU_DIAL_TONE) { dprintf(tu->fileno, "DIAL TONE\r\n");
        } else if(tu->state == TU_RING_BACK) { dprintf(tu->fileno, "RING BACK\r\n");
        } else if(tu->state == TU_BUSY_SIGNAL){ dprintf(tu->fileno, "BUSY SIGNAL\r\n");
        } else if(tu->state == TU_CONNECTED) { dprintf(tu->fileno, "CONNECTED %d\r\n", tu->peer_tu_fileno);
        } else if(tu->state == TU_ERROR) { dprintf(tu->fileno, "ERROR\r\n"); }
    }
    V(&mutex);
    return 0;
}

int tu_hangup(TU *tu){
    P(&mutex);
    if(tu == NULL){
        V(&mutex);
        return -1;
    }
    debug("hangup: tu FD %d ->state initially is %d",tu->fileno, tu->state);
    if(tu->state == TU_CONNECTED){ //if CONNECTED
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        //set peer tu to DIAL TONE
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_DIAL_TONE;
        //write confirmation messages
        dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
        dprintf(tu->peer_tu_fileno, "DIAL TONE\r\n");
        V(&mutex);
        return 0;
    }else if(tu->state == TU_RING_BACK){ //if RING BACK
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        //set peer tu to ON HOOK
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_ON_HOOK;
        //send confirmation messages
        dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
        dprintf(tu->peer_tu_fileno, "ON HOOK %d\r\n", tu->peer_tu_fileno);;
        V(&mutex);
        return 0;
    }else if(tu->state == TU_RINGING){ //if RINGING
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        //set peer tu to DIAL TONE
        pbx->TU_list[tu->peer_tu_fileno]->state = TU_DIAL_TONE;
        //write confirmation messages
        dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
        dprintf(tu->peer_tu_fileno, "DIAL TONE\r\n");
        V(&mutex);
        return 0;
    }else if(tu->state == TU_DIAL_TONE || tu->state == TU_BUSY_SIGNAL || tu->state == TU_ERROR) {
        //set tu to ON HOOK
        tu->state = TU_ON_HOOK;
        dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
    }else if(tu->state == TU_ON_HOOK) { //if nothing else, keep current state
        dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
    }

    V(&mutex);
    return 0;
}

int tu_dial(TU *tu, int ext){
    P(&mutex);
    if(tu == NULL || ext < 0) {
        V(&mutex);
        return -1;
    }
    if(tu->state == TU_DIAL_TONE) { //if DIAL TONE
        //check that extension number is registered
        if(pbx->TU_list[ext] == NULL) {
            //set state to be ERROR
            debug("dial: error detected & state set");
            tu->state = TU_ERROR;
            dprintf(tu->fileno, "ERROR\r\n");
        }else if(pbx->TU_list[ext]->state == TU_ON_HOOK) { //if peer = ON HOOK
            tu->peer_tu_fileno = ext; //set peer fileno
            pbx->TU_list[ext]->peer_tu_fileno = tu_fileno(tu); //set peer's peer fileno
            //set dialing tu(tu) to RING BACK
            tu->state = TU_RING_BACK;
            //set dialed tu(peer tu) to RINGING
            pbx->TU_list[ext]->state = TU_RINGING;
            //write confirmation messages
            dprintf(tu->fileno, "RING BACK\r\n");
            dprintf(pbx->TU_list[tu->peer_tu_fileno]->fileno, "RINGING\r\n");
            V(&mutex);
            return 0;
        }else { //if peer != DIAL TONE
            tu->state = TU_BUSY_SIGNAL;
            dprintf(tu->fileno, "BUSY SIGNAL\r\n");
        }
    } else { //if not DIAL TONE
        //leave default state alone
        if(tu->state == TU_ON_HOOK){ dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
        }else if(tu->state == TU_RINGING) { dprintf(tu->fileno, "RINGING\r\n");
        }else if(tu->state == TU_RING_BACK) { dprintf(tu->fileno, "RING BACK\r\n");
        }else if(tu->state == TU_BUSY_SIGNAL) { dprintf(tu->fileno, "BUSY SIGNAL\r\n");
        }else if(tu->state == TU_CONNECTED) { dprintf(tu->fileno, "CONNECTED %d\r\n", tu->peer_tu_fileno);
        }else if(tu->state == TU_ERROR) { dprintf(tu->fileno, "ERROR\r\n"); }
    }

    V(&mutex);
    return 0;
}

int tu_chat(TU *tu, char *msg){
    if(tu->state != TU_CONNECTED || msg == NULL) {
        if(tu->state == TU_ON_HOOK){ dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
        }else if(tu->state == TU_RINGING) { dprintf(tu->fileno, "RINGING\r\n");
        }else if(tu->state == TU_DIAL_TONE) { dprintf(tu->fileno, "DIAL TONE\r\n");
        }else if(tu->state == TU_RING_BACK) { dprintf(tu->fileno, "RING BACK\r\n");
        }else if(tu->state == TU_BUSY_SIGNAL) { dprintf(tu->fileno, "BUSY SIGNAL\r\n");
        }else if(tu->state == TU_ERROR) { dprintf(tu->fileno, "ERROR\r\n"); }
        return -1;
    }
    //print message
    char *appended_msg = strcat(msg, "\r\n\0");
    dprintf(tu->peer_tu_fileno, "CHAT %s", appended_msg);
    //print current state
    if(tu->state == TU_ON_HOOK){ dprintf(tu->fileno, "ON HOOK %d\r\n", tu->fileno);
    }else if(tu->state == TU_RINGING) { dprintf(tu->fileno, "RINGING\r\n");
    }else if(tu->state == TU_DIAL_TONE) { dprintf(tu->fileno, "DIAL TONE\r\n");
    }else if(tu->state == TU_RING_BACK) { dprintf(tu->fileno, "RING BACK\r\n");
    }else if(tu->state == TU_BUSY_SIGNAL) { dprintf(tu->fileno, "BUSY SIGNAL\r\n");
    }else if(tu->state == TU_CONNECTED) { dprintf(tu->fileno, "CONNECTED %d\r\n", tu->peer_tu_fileno);
    }else if(tu->state == TU_ERROR) { dprintf(tu->fileno, "ERROR\r\n"); }

    return 0;
}