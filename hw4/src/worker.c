#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include "polya.h"

volatile sig_atomic_t SIGHUP_cancel = 0; //if non-zero, cancel current solution attempt

//handler to exit gracefully
void SIGTERM_handler(int sig) {
    exit(EXIT_SUCCESS);
}

//sets up flag to cancel current solution attempt
void SIGHUP_handler(int sig){
    SIGHUP_cancel = 1;
}

/*
 * worker
 * (See polya.h for specification.)
 */
int worker(void) {
    //Initialization
    if(signal(SIGTERM, SIGTERM_handler) == SIG_ERR) //handle SIGTERM
        printf("SIGTERM_handler returned SIG_ERR");
    if(signal(SIGHUP, SIGHUP_handler) == SIG_ERR) //handle SIGHUP
        printf("SIGHUP_handler returned SIG_ERR");
    kill(getpid(),SIGSTOP); //send SIGSTOP to itself
    //read problems continously
    while(1) {
        struct problem *new_problem = malloc(sizeof(struct problem)); //allocate space for header
        fread(new_problem, sizeof(struct problem), 1, stdin); //read header into problem
        new_problem = realloc(new_problem, new_problem->size); //reallocing the correct size
        fread(new_problem, new_problem->size - sizeof(struct problem), 1, stdin); //reading the rest of the problem (size - header)
        fflush(stdin);
        //solve problem & free problem malloc
        struct result *new_result = solvers[new_problem->type].solve(new_problem, &SIGHUP_cancel);
        free(new_problem);
        //write result & free result malloc
        fwrite(new_result, new_result->size, 1, stdout);
        free(new_result);
        //SIGSTOP and wait for master to send new problem
        kill(getpid(),SIGSTOP);
    }

    return EXIT_FAILURE;
}
