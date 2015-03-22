/*==============================
==  PRANJAL PANDEY 12CS30026  ==
==  NEVIN VALSARAJ 12CS10032  ==
==  Assignment 3b             ==
===============================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define READY 100

int main() {
    srand(time(NULL));
    int pipe_child_parent[2][2];
    pid_t fork_pid[2];
    int i;
    int status;
    int length;
    char child_argument[80];
    float child1_score = 0;
    float child2_score = 0;

    int reply[2];

    for (i = 0; i < 2; i++) {
        if (pipe(pipe_child_parent[i]) < 0) {
            perror("Failed to allocate pipes");
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < 2; i++) {
        fork_pid[i] = fork();
        if (fork_pid[i] == 0) {
            close(pipe_child_parent[i][0]);
            sprintf(child_argument, "%d", pipe_child_parent[i][1]);
            char *cmd[] = {"./child", child_argument, 0};
            execvp("./child", cmd);
        }
    }
    printf("we have created\n");
    reply[0] = reply[1] = -1;
    for (i = 0; i < 2; i++) {
        while (reply[i] == -1) {
            length = read(pipe_child_parent[i][0], &reply[i], sizeof(reply[i]));
        }
        if (reply[i] == READY) {
            printf("-- We have established communication with player %d\n",i+1);
        }
    }

    /* The following code is used
    1 -- paper
    2 -- scissor
    3 -- rock
    */
    // Parent sends the ready signal - SIGUSR1 to the children
    while (child1_score < 10 && child2_score < 10) {
        for (i = 0; i < 2; i++)
            reply[i]=-1;
        // since none of the child is the winner, lets ask both of them for their choice.
        for (i = 0; i < 2; i++) {
            kill(fork_pid[i], SIGUSR1);
            while(reply[i] == -1) {
                length = read(pipe_child_parent[i][0], &reply[i], sizeof(reply[i]));
            }
            sleep(3);
        }

        switch(reply[0]) {
            case 1:
                if (reply[1] == 1) {
                    printf("-- Both of them chose paper -- Nobody won\n");
                    child1_score+=0.5;
                    child2_score+=0.5;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                } else if (reply[1] == 2) {
                    printf("-- Player 2 chose scissors and Player 1 chose paper -- Player 2 won\n");
                    child1_score+=0;
                    child2_score+=1;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                } else if (reply[1] == 3) {
                    printf("-- Player 2 chose rock and Player 1 chose paper -- Player 1 won\n");
                    child1_score+=1;
                    child2_score+=0;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                }
            case 2:
                if (reply[1] == 1) {
                    printf("-- Player 2 chose paper and Player 1 chose scissors -- Player 1 won\n");
                    child1_score+=1;
                    child2_score+=0;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                } else if (reply[1] == 2) {
                    printf("-- Both of them chose scissors -- Nobody won\n");
                    child1_score+=0.5;
                    child2_score+=0.5;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                } else if (reply[1] == 3) {
                    printf("-- Player 2 chose rock and Player 1 chose scissors -- Player 2 won\n");
                    child1_score+=0;
                    child2_score+=1;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                }
            case 3:
                if (reply[1] == 1) {
                    printf("-- Player 2 chose paper and Player 1 chose rock -- Player 2 won\n");
                    child1_score+=0;
                    child2_score+=1;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                }
                else if (reply[1] == 2) {
                    printf("-- Player 2 chose scissors and Player 1 chose rock -- Player 1 won\n");
                    child1_score+=1;
                    child2_score+=0;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                } else if (reply[1] == 3) {
                    printf("-- Both of them chose rock -- Nobody won\n");
                    child1_score+=0.5;
                    child2_score+=0.5;
                    printf("-- Current Score: Player 1 = %f\t Player 2 = %f\n",child1_score,child2_score);
                    break;
                }
        }
    }

    if (child1_score != child2_score) {
        if (child1_score > child2_score) {
            printf("-- Player 1 won the game quite clearly\n");
        } else {
            printf("-- Player 2 won the game quite clearly\n");
        }
    } else {
        printf("The scores are tied. We need to resolve the tie amicably\n");
        reply[0] = rand()%1000;
        reply[1] = rand()%1000;
        if (reply[0] > reply[1]) {
            printf("-- Player 1 won the tie\n");
        } else {
            printf("-- Player 2 won the tie\n");
        }
    }

    printf("----- Parent sends kill signals to all children\n");
    for (i = 0; i < 2; i++) {
        kill(fork_pid[i], SIGUSR2);
    }
    for (i = 0; i < 2; i++) {
        close(pipe_child_parent[i][0]);
    }
    for (i = 0; i < 2; i++) {
        waitpid(fork_pid[i], &status, 0);
        printf("----- Player %d terminates\n", i+1);
    }
    return 0;
}
