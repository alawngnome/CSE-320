#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include "polya.h"

volatile sig_atomic_t SIGHUP_cancel = 0; //if non-zero, cancel current solution attempt
volatile sig_atomic_t SIGTERM_cancel = 0; //if non-zero, break out of loop

//handler to exit gracefully
void SIGTERM_handler(int sig) {
    SIGTERM_cancel = 1;
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
    signal(SIGTERM, SIGTERM_handler); //handle SIGTERM
    signal(SIGHUP, SIGHUP_handler); //handle SIGHUP
    kill(getpid(), SIGSTOP); //send SIGSTOP to itself
    //read problems continously
    while(SIGTERM_cancel == 0) {
        debug("BEGINNING LOOP\n");
        //read problem
        struct problem *new_problem = malloc(sizeof(struct problem)); //allocate space for header
        fread(new_problem, sizeof(struct problem), 1, stdin); //read header into problem
        new_problem = realloc(new_problem, new_problem->size); //reallocing the correct size
        fread(new_problem, new_problem->size - sizeof(struct problem), 1, stdin); //reading the rest of the problem (size - header)
        fflush(stdin);
        //solve problem & free problem malloc
        debug("CALCULATING RESULT...");
        struct result *new_result = solvers[new_problem->type].solve(new_problem, &SIGHUP_cancel);
        debug("RESULT CALCULATED\n");
        free(new_problem);
        //if SIGTERM don't write solution and break
        if(SIGTERM_cancel){
            debug("SIGTERM CANCEL CALLED");
            free(new_result);
            break;
        }
        //set failed test result if SIGHUP or solve was cancelled
        if(new_result == NULL || SIGHUP_cancel != 0){
            //create failed result
            debug("WRITING FAILED RESULT\n");
            struct result *brand_new_result = malloc(sizeof(struct result));
            brand_new_result->size = sizeof(struct result);
            brand_new_result->id = 0;
            brand_new_result->failed = 1;
            //write result & free result malloc
            fwrite(brand_new_result, brand_new_result->size, 1, stdout);
            fflush(stdout);
            free(brand_new_result);
            free(new_result);
        } else {
        //write result & free result malloc
            debug("WRITING SUCCESSFUL RESULT\n");
            fwrite(new_result, new_result->size, 1, stdout);
            fflush(stdout);
            free(new_result);
        }
        debug("HOW DOES THIS EVEN WORK\n");
        //SIGSTOP and wait for master to send new problem
        SIGHUP_cancel = 0; //reset SIGHUP flag
        kill(getpid(),SIGSTOP);
    }
    debug("WHAT ABOUT OUTSIDE THE LOOP\n");
    //return EXIT_SUCCESS;
    exit(EXIT_SUCCESS);
}
