#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include "polya.h"

volatile sig_atomic_t SIGCHLD_flag = 0; //when SIGCHLD is raised

//array of worker states
volatile sig_atomic_t *worker_states;

void SIGCHLD_handler(int sig) {
    SIGCHLD_flag = 1;
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
    int pid_array[workers]; //create array of IDs
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
        }
    }
    //loop until terminate
    while(1) {
        //find idle workers in list of workers
        for(int i = 0; i < workers; i++) {
            if(worker_states[i] == WORKER_IDLE){ //if idle worker found
                //creating the problem
                struct problem *new_problem = get_problem_variant(workers, pid_array[i]);
                //pipe header of the problem
                fwrite(new_problem, sizeof(struct problem), 1, fdopen(problem_fd[2*i+1], "w"));
                //pipe rest of problem
                fwrite(new_problem->data, new_problem->size - sizeof(struct problem), 1, fdopen(problem_fd[2*i+1], "w"));

                //CHANGE STATE TO BE NOT IDLE!!!!!!!!!!! use kill()
                kill(pid_array[i], SIGCONT);
            }
        }
    }

    return EXIT_FAILURE;
}
