/*==============================
==  PRANJAL PANDEY 12CS30026  ==
==  NEVIN VALSARAJ 12CS10032  ==
==  Assignment 3b             ==
===============================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define READY 100

int id;
int child_write;

void exit_child_process() {
    close(child_write);
    exit(EXIT_SUCCESS);
}

void generate_send_random() {
    srand((unsigned int) time(NULL));
    int random_number = rand()%3 + 1;
    write(child_write, &random_number, sizeof(random_number));
    printf("-- I am sending %d as my move\n", random_number);
}

int main(int argc, char *argv[]) {
    srand((unsigned int) time(NULL));

    child_write = atoi(argv[1]);

    int send_signal;
    send_signal = READY;
    printf("----- Player sends READY\n");
    write(child_write, &send_signal, sizeof(send_signal));
    struct sigaction sa_generate;
    struct sigaction sa_end;

    sa_generate.sa_handler = generate_send_random;
    sa_generate.sa_flags = 0;
    sigemptyset(&sa_generate.sa_mask);

    sa_end.sa_handler = exit_child_process;
    sa_end.sa_flags = 0;
    sigemptyset(&sa_end.sa_mask);

    while (1) {
        if (sigaction(SIGUSR1, &sa_generate, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }
        if (sigaction(SIGUSR2, &sa_end, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }
    }
}
