#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "debug.h"
#include "polya.h"

//volatile sig_atomic_t SIGCHLD_flag = 0; //when SIGCHLD is raised

volatile sig_atomic_t *worker_states; //worker states
volatile sig_atomic_t *pid_array; //array of worker IDs
volatile sig_atomic_t *workers_global; //global copy of workers

volatile sig_atomic_t STOPPED_flag = 0; //flag if at least one worker found a SOLUTION

void SIGCHLD_handler(int sig) {
    int worker_ID;
    int status;
    //continously handle SIGCHLD signals until no more appear
    while(1) {
        if((worker_ID = waitpid(-1, &status, 0|WNOHANG|WUNTRACED|WCONTINUED)) == 0){
            return; //leave SIGCHLD handler if WNOHANG triggers
        }
        //Find the index of the worker
        for(int i = 0; i < *workers_global; i++) {
            if(pid_array[i] == worker_ID){
                //determine what stage the worker is in
                //WORKER_STARTED is set in master
                if(worker_states[i] == WORKER_STARTED && WIFSTOPPED(status)){
                    sf_change_state(pid_array[i], WORKER_STARTED, WORKER_IDLE);
                    worker_states[i] = WORKER_IDLE;
                }
                //WORKER_CONTINUED is set in master
                else if(worker_states[i] == WORKER_CONTINUED && WIFCONTINUED(status)){
                    sf_change_state(pid_array[i], worker_states[i], WORKER_RUNNING);
                    worker_states[i] = WORKER_RUNNING;
                }
                else if(worker_states[i] == WORKER_RUNNING && WIFSTOPPED(status)) {
                    sf_change_state(pid_array[i], worker_states[i], WORKER_STOPPED);
                    worker_states[i] = WORKER_STOPPED;
                    STOPPED_flag = 1;
                }
                else if(worker_states[i] == WORKER_IDLE && WIFEXITED(status)) {
                    sf_change_state(pid_array[i], worker_states[i], WORKER_EXITED);
                    worker_states[i] = WORKER_EXITED;
                }
                else if(WIFSIGNALED(status)) { //if not any of the above, must be aborted
                    sf_change_state(pid_array[i], worker_states[i], WORKER_EXITED);
                    worker_states[i] = WORKER_ABORTED;
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
    sf_start(); //called when master has just begun to execute
    //Initializing
    signal(SIGCHLD, SIGCHLD_handler); //SIGCHLD handler
    signal(SIGPIPE, SIGPIPE_handler); //SIGPIPE handler
    int pid[workers]; //multiple pid for multiple processes
    int problem_fd[workers*2]; //2 fd per process (problem)
    int result_fd[workers*2]; //2 fd per process (worker)
    worker_states = malloc(sizeof(sig_atomic_t) * workers); //create array of states
    pid_array = malloc(sizeof(int)*workers); //create array of worker IDs
    struct problem *problem_array[workers];
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
            dup2(problem_fd[2*i], STDIN_FILENO); //redirecting stdin to pipe read
            //result pipe redirection
            close(result_fd[2*i]); //close read side of result pipe
            dup2(result_fd[2*i+1], STDOUT_FILENO); //redirecting stdout to pipe write
            //execute worker
            execl("bin/polya_worker", "bin/polya_worker", NULL);
        } else { //master / parent
            close(problem_fd[2*i]); //close read side of problem pipe
            close(result_fd[2*i+1]); //close write side of problem pipe
            pid_array[i] = pid[i]; //store child id into pid_array
            //set worker state to STARTED
            sf_change_state(pid_array[i], 0, WORKER_STARTED);
            worker_states[i] = WORKER_STARTED;
        }
    }

    //MAIN LOOP - loops until no more problem to execute
    while(1) {
        int no_new_problem = 0; //flag if get_problem_variant returns null

        //find IDLE workers in list of workers & give it a job
        for(int i = 0; i < workers; i++) {
            if(worker_states[i] == WORKER_IDLE){ //if idle worker found
                //creating the problem
                struct problem *new_problem = get_problem_variant(workers, i);
                //store it in problem array
                problem_array[i] = new_problem;
                //if no more problem variants can be created
                if(new_problem == NULL) {
                    no_new_problem = 1;
                    break;
                }
                sf_send_problem(pid_array[i], new_problem); //called right before master writes problem
                //pipe problem into problem pipe
                fwrite(new_problem, new_problem->size, 1, fdopen(problem_fd[2*i+1], "w"));
                fflush(fdopen(problem_fd[2*i], "w")); //HOW DOES THIS WORK???
                //set continued worker state
                sf_change_state(pid_array[i], WORKER_IDLE, WORKER_CONTINUED);
                worker_states[i] = WORKER_CONTINUED;
                //sending SIGCONT signal
                kill(pid_array[i], SIGCONT);
            }
        }

        //if solution found, send SIGHUP signal to all other workers
        //find STOPPED workers in list of workers & read result from pipe
        if(STOPPED_flag) {
            for(int i = 0; i < workers; i++) {
                if(worker_states[i] == WORKER_STOPPED) { //if pending result found
                    //set up result struct
                    struct result *new_result = malloc(sizeof(struct result));
                    //read problem
                    FILE *read_FD = fdopen(result_fd[2*i], "r");
                    fread(new_result, sizeof(struct result), 1, read_FD);
                    new_result = realloc(new_result, new_result->size);
                    //debug("Read the first part of result in...\n");
                    fread(new_result->data, new_result->size - sizeof(struct result), 1, read_FD);
                    //debug("...Read the second part of result in?\n");
                    fflush(fdopen(result_fd[2*i], "r"));
                    sf_recv_result(pid_array[i], new_result); //called after problem was read
                    if(post_result(new_result, problem_array[i])){
                        debug("YAY SOLUTION FOUND\n");
                    }
                    //solution posted, worker goes to IDLE
                    sf_change_state(pid_array[i], WORKER_STOPPED, WORKER_IDLE);
                    worker_states[i] = WORKER_IDLE;
                }
                else if(worker_states[i] == WORKER_RUNNING) { //if other workers are running with a solution already found
                    kill(pid_array[i], SIGHUP); //send a SIGHUP signal to cancel
                }
            }
        }
        STOPPED_flag = 0; //reset STOPPED flag

        //end workers and master
        if(no_new_problem){
            int abnormal_termination = 0; //flag, 1 if abnormal termination
            //if non-idle worker found
            for(int i = 0; i < workers; i++) {
                if(worker_states[i] != WORKER_IDLE){
                    kill(pid_array[i], SIGHUP); //end solving if not already IDLE
                }
            }
            //send SIGTERM signal to all workers
            for(int i = 0; i < workers; i++) {
                //send SIGTERM signal & exit
                if(kill(pid_array[i], SIGTERM) == -1) { //if error
                    abnormal_termination = 1; //abnormal termination found
                }
            }
            //called as master is about to terminate
            sf_end();
            //if a worker exited with EXIT_FAILURE
            if(abnormal_termination){
                exit(EXIT_FAILURE);
            }
            //otherwise, return with EXIT_SUCCESS
            exit(EXIT_SUCCESS);
        }
    }
    return EXIT_FAILURE;
}
