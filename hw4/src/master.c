#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "debug.h"
#include "polya.h"

sig_atomic_t *worker_states; //worker states
sig_atomic_t *pid_array; //array of worker IDs
sig_atomic_t *workers_global; //global copy of workers

volatile sig_atomic_t STOPPED_flag = 0; //flag if at least one worker found a SOLUTION

void SIGCHLD_handler(int sig) {
    int worker_ID;
    int status;
    //continously handle SIGCHLD signals until no more appear
    while((worker_ID = waitpid(-1, &status, 0|WNOHANG|WUNTRACED|WCONTINUED)) > 0) {
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
                else if((worker_states[i] == WORKER_RUNNING && WIFSTOPPED(status)) ||
                    (worker_states[i] == WORKER_CONTINUED && WIFSTOPPED(status))) {
                    sf_change_state(pid_array[i], worker_states[i], WORKER_STOPPED);
                    worker_states[i] = WORKER_STOPPED;
                    STOPPED_flag = 1;
                }
                else if(WIFEXITED(status)) {
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
    //Initializing handlers
    signal(SIGCHLD, SIGCHLD_handler); //SIGCHLD handler
    signal(SIGPIPE, SIGPIPE_handler); //SIGPIPE handler
    //Intializing pipe fds
    int pid[workers]; //multiple pid for multiple processes
    int problem_fd[workers*2]; //2 fd per process (problem)
    int result_fd[workers*2]; //2 fd per process (worker)
    worker_states = malloc(sizeof(sig_atomic_t) * workers); //create array of states
    pid_array = malloc(sizeof(int)*workers); //create array of worker IDs
    struct problem *problem_array[workers];
    //set up signal blocking
    sigset_t SIGCHLD_mask, prev_mask;
    sigemptyset(&SIGCHLD_mask);
    sigaddset(&SIGCHLD_mask, SIGCHLD);
    //sigprocmask(SIG_BLOCK, &SIGCHLD_mask, &prev_mask); //masking SIGCHLD
    //sigprocmask(SIG_SETMASK, &prev_mask, NULL); //unmasking SIGCHLD
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
            sigprocmask(SIG_BLOCK, &SIGCHLD_mask, &prev_mask); //masking SIGCHLD
            worker_states[i] = WORKER_STARTED;
            sigprocmask(SIG_SETMASK, &prev_mask, NULL); //unmasking SIGCHLD
        }
    }
    //wait for all workers to turned IDLE
    for(int i = 0; i < workers; i++){
        if(worker_states[i] == WORKER_STARTED) {
            pause();
        }
    }
    int no_new_problem = 0; //flag if no new problems can be created
    //MAIN LOOP - loops until no more problem to execute
    while(1) {
        //find IDLE workers in list of workers & give it a job
        for(int i = 0; i < workers; i++) {
            if(worker_states[i] == WORKER_IDLE){ //if idle worker found
                //creating the problem
                struct problem *new_problem;
                if(no_new_problem == 0){
                    new_problem = get_problem_variant(workers, i);
                }
                //if no more problem variants can be created
                if(new_problem == NULL) {
                    no_new_problem = 1;
                    for(int i = 0; i < workers; i++) {
                        if(worker_states[i] != WORKER_IDLE){ //if not all idle yet
                            goto process_result;
                        }
                    }
                    goto no_new_problem; //if all idle go to terminate
                }
                //store it in problem array
                problem_array[i] = new_problem;
                sf_send_problem(pid_array[i], new_problem); //called right before master writes problem
                //pipe problem into problem pipe
                FILE *write_FD = fdopen(problem_fd[2*i+1], "w");
                fwrite(new_problem, new_problem->size, 1, write_FD);
                fflush(write_FD); //HOW DOES THIS WORK???
                //set continued worker state
                sf_change_state(pid_array[i], WORKER_IDLE, WORKER_CONTINUED);
                sigprocmask(SIG_BLOCK, &SIGCHLD_mask, &prev_mask); //masking SIGCHLD
                worker_states[i] = WORKER_CONTINUED;
                sigprocmask(SIG_SETMASK, &prev_mask, NULL); //unmasking SIGCHLD
                //sending SIGCONT signal
                kill(pid_array[i], SIGCONT);
            }
        }

        process_result:
        //if solution found, send SIGHUP signal to all other workers
        //find STOPPED workers in list of workers & read result from pipe
        if(STOPPED_flag) {
            for(int i = 0; i < workers; i++) {
                if(worker_states[i] == WORKER_STOPPED) { //if pending result found
                    //set up result struct
                    struct result *new_result = malloc(sizeof(struct result));
                    //read result
                    FILE *read_FD = fdopen(result_fd[2*i], "r");
                    fread(new_result, sizeof(struct result), 1, read_FD);
                    new_result = realloc(new_result, new_result->size);
                    fread(new_result->data, new_result->size - sizeof(struct result), 1, read_FD);
                    sf_recv_result(pid_array[i], new_result); //called after problem was read
                    struct problem *temp_problem = malloc(sizeof(struct problem)); //temp problem to maintain id
                    //if successful result found
                    if(new_result->failed == 0 && problem_array[i] != NULL) {
                        temp_problem->id = problem_array[i]->id; //set temp ID for following workers to submit to
                        post_result(new_result, problem_array[i]);
                        //find all problem variants in problem array and NULL them
                        for(int i = 0; i < workers; i++){
                            if(!(problem_array[i] == NULL) && //if already null pass over
                            problem_array[i]->id == temp_problem->id)
                                problem_array[i] = NULL;
                        }
                    }
                    //if another successful result found but problem solved
                    if(problem_array[i] == NULL) {
                        new_result->failed = 1;
                        post_result(new_result, temp_problem);
                    }
                    free(temp_problem);
                    free(new_result);
                    //solution posted, worker goes to IDLE
                    sf_change_state(pid_array[i], WORKER_STOPPED, WORKER_IDLE);
                    sigprocmask(SIG_BLOCK, &SIGCHLD_mask, &prev_mask); //masking SIGCHLD
                    worker_states[i] = WORKER_IDLE;
                    sigprocmask(SIG_SETMASK, &prev_mask, NULL); //unmasking SIGCHLD
                }
                else if(worker_states[i] == WORKER_RUNNING) { //if calculating result found
                    sf_cancel(pid_array[i]); //current problem solve cancelled
                    kill(pid_array[i], SIGHUP); //send a SIGHUP signal to cancel
                }
            }
        }
        STOPPED_flag = 0; //reset STOPPED flag
    }

    //end workers and master
    no_new_problem:
        //send SIGTERM signal to all workers
        for(int i = 0; i < workers; i++) {
            //send SIGTERM signal
            //sf_change_state(pid_array[i], WORKER_CONTINUED, WORKER_EXITED);
            kill(pid_array[i], SIGTERM);
            //set CONT signal
            sf_change_state(pid_array[i], WORKER_IDLE, WORKER_CONTINUED);
            sigprocmask(SIG_BLOCK, &SIGCHLD_mask, &prev_mask); //masking SIGCHLD
            worker_states[i] = WORKER_CONTINUED; //don't do this so HANDLER sees the state as IDLE to terminate
            sigprocmask(SIG_SETMASK, &prev_mask, NULL); //unmasking SIGCHLD
            kill(pid_array[i], SIGCONT); //send a continue so worker can process it
        }
        //wait for all workers to become exited
        while(1) {
            int full_iteration = 1;
            for(int i = 0; i < workers; i++) {
                if(worker_states[i] != WORKER_EXITED) {
                    full_iteration = 0; //if non-exited found reset
                }
            }
            if(full_iteration == 1){
                break;
            }
        }
        //free malloced data
        free(worker_states);
        free(pid_array);
        free(workers_global);
        //called as master is about to terminate
        sf_end();
        //if a worker exited with EXIT_FAILURE
        for(int i = 0; i < workers; i++){
            if(worker_states[i] == WORKER_ABORTED) {
                exit(EXIT_FAILURE);
            }
        }
        //otherwise, return with EXIT_SUCCESS
        exit(EXIT_SUCCESS);
}
