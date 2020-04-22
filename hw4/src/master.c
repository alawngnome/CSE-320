#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "debug.h"
#include "polya.h"

//volatile sig_atomic_t SIGCHLD_flag = 0; //when SIGCHLD is raised

volatile sig_atomic_t *worker_states; //worker states
volatile sig_atomic_t *pid_array; //array of worker IDs
volatile sig_atomic_t *workers_global;

void SIGCHLD_handler(int sig) {
    int worker_ID;
    int status;
    //continously handle SIGCHLD signals until no more appear
    while(1) {
        if((worker_ID = waitpid(-1, &status, 0|WNOHANG|WUNTRACED|WIFCONTINUED(status))) == 0){
            return; //leave SIGCHLD handler if WNOHANG triggers
        }
        //Find the index of the worker
        for(int i = 0; i < *workers_global; i++) {
            if(pid_array[i] == worker_ID){
                //determine what stage the worker is in
                //WORKER_STARTED is set up in master
                if(worker_states[i] == WORKER_STARTED){
                    worker_states[i] = WORKER_IDLE;
                }
                //WORKER_CONTINUED is set up in master
                else if(worker_states[i] == WORKER_CONTINUED){
                    worker_states[i] = WORKER_RUNNING;
                }
                else if(worker_states[i] == WORKER_RUNNING) {
                    worker_states[i] = WORKER_STOPPED;
                }
                else if(worker_states[i] == WORKER_IDLE) {
                    worker_states[i] = WORKER_EXITED;
                }
            }
        }
    }
}
void SIGPIPE_handler(int sig) {
    //do nothing
}

/*
 * master
 * (See polya.h for specification.)
 */
int master(int workers) {
    //Initializing
    signal(SIGCHLD, SIGCHLD_handler); //SIGCHLD handler
    signal(SIGPIPE, SIGPIPE_handler); //SIGPIPE handler
    int pid[workers]; //multiple pid for multiple processes
    int problem_fd[workers*2]; //2 fd per process (problem)
    int result_fd[workers*2]; //2 fd per process (worker)
    worker_states = malloc(sizeof(sig_atomic_t) * workers); //create array of states
    pid_array = malloc(sizeof(int)*workers); //create array of worker IDs
    //make a global copy of workers
    workers_global = malloc(sizeof(workers));
    *workers_global = workers;
    //creating pipes, workers and redirection
    for(int i = 0; i < workers; i++) {
        //creating pipes
        pipe(&problem_fd[2*i]); //master -> worker / problem pipe
        pipe(&result_fd[2*i]); //master <- worker / worker pipe
        //creating workers
        if((pid[i] = fork()) == 0) { //worker / child
            //problem pipe redirection
            close(problem_fd[2*i+1]); //close write side of problem pipe
            dup2(problem_fd[2*i], 0); //redirecting stdin to pipe read
            //result pipe redirection
            close(result_fd[2*i]); //close read side of result pipe
            dup2(result_fd[2*i+1], 1); //redirecting stdout to pipe write
            //execute worker
            execl("bin/polya_worker", "bin/polya_worker", NULL);
        } else { //master / parent
            close(problem_fd[2*i]); //close read side of problem pipe
            close(result_fd[2*i+1]); //close write side of problem pipe
            pid_array[i] = pid[i]; //store child id into pid_array
            //set worker state to STARTED
            worker_states[i] = WORKER_STARTED;
        }
    }

    //MAIN LOOP - loops until no more problem to execute
    while(1) {
        int no_new_problem = 0; //flag if get_problem_variant returns null
        struct problem *new_problem; //creating problem

        //find IDLE workers in list of workers & give it a job
        for(int i = 0; i < workers; i++) {
            if(worker_states[i] == WORKER_IDLE){ //if idle worker found
                //creating the problem
                new_problem = get_problem_variant(workers, i);
                //if no more problem variants can be created
                if(new_problem == NULL) {
                    no_new_problem = 1;
                    break;
                }
                //pipe header of the problem
                fwrite(new_problem, sizeof(struct problem), 1, fdopen(problem_fd[2*i+1], "w"));
                //pipe rest of problem
                fwrite(new_problem->data, new_problem->size - sizeof(struct problem), 1, fdopen(problem_fd[2*i+1], "w"));
                //set continued worker state
                worker_states[i] = WORKER_CONTINUED;
                //sending SIGCONT signal
                kill(pid_array[i], SIGCONT);
            }
        }
        //find STOPPED workers in list of workers & read result from pipe
        for(int i = 0; i < workers; i++) {
            if(worker_states[i] == WORKER_STOPPED) { //if pending result found
                //set up result struct
                struct result *new_result = malloc(sizeof(struct result));
                //read problem
                fread(new_result, sizeof(struct result), 1, fdopen(problem_fd[2*i], "r"));
                new_result = realloc(new_result, new_result->size);
                fread(new_result->data, new_result->size - sizeof(struct result), 1, fdopen(problem_fd[2*i], "r"));
                fflush(fdopen(problem_fd[2*i], "r"));
                if(post_result(new_result, new_problem)){
                    debug("YAY SOLUTION FOUND\n");
                }
            }
        }

        //end workers and master
        if(no_new_problem){
            //if non-idle worker found
            for(int i = 0; i < workers; i++) {
                if(worker_states[i] != WORKER_IDLE){
                    kill(pid_array[i], SIGHUP); //send the SIGHUP
                }
            }
            //send SIGTERM signal to all workers
            for(int i = 0; i < workers; i++) {
                //send SIGTERM signal & exit
                kill(pid_array[i], SIGTERM);
            }
            //if all workers exit with EXIT_SUCCESSS
            exit(EXIT_SUCCESS);
            //if otherwise
            exit(EXIT_FAILURE);
        }
    }
    return EXIT_FAILURE;
}
