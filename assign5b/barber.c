#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>

//#include "shop.h"

#define BARBER_KEY 1000
#define CUSTOMER_KEY 1001
#define MUTEX_KEY 1002

int access_semaphores();
int wait_on(int);
int signal_to(int);
void kill_barber(int);

typedef struct sembuf sem_buffer;

int customers_waiting;
int barber_idle;
int mutex;
pid_t start;

int main(int argc,char** argv){

	start = atoi(argv[1]);
	signal(SIGINT,kill_barber);
	access_semaphores();
	while(1){
		fprintf(stderr, "Barber:Trying to acquire a customer else I will sleep\n" );
		wait_on(customers_waiting);
		fprintf(stderr,"Barber:Awake,Try to access waiting room_chairs else sleep\n");
		wait_on(mutex);
		fprintf(stderr, "Barber:Awake,One waiting room chair is free now\n");
		kill(start,SIGUSR2);
		fprintf(stderr, "Barber:I am ready to cut\n");
		signal_to(barber_idle);
		fprintf(stderr, "Barber:Releasing access to Waiting Room\n");
		signal_to(mutex);
		fprintf(stderr, "Barber: Giving haircut\n");
	}
	return 0;
}

int access_semaphores(){

	if((customers_waiting = semget(CUSTOMER_KEY,1, 0666)) < 0){
		perror("Semaphore customers created");
		exit(1);
	}
	if((barber_idle = semget(BARBER_KEY,1, 0666)) < 0){
		perror("Semaphore barber created");
		exit(1);
	}
	if((mutex = semget(MUTEX_KEY,1, 0666)) < 0){
		perror("Semaphore mutex created");
		exit(1);
	}
	return 0;
}


int wait_on(int sem_id){
	sem_buffer operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = -1;
	operations[0].sem_flg = SEM_UNDO;

	if(semop(sem_id,operations,1) < 0){
		perror("semop wait_on");
		exit(1);
	}
	return 0;
}

int signal_to(int sem_id){
	sem_buffer operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = 1;
	operations[0].sem_flg = SEM_UNDO;

	if(semop(sem_id,operations,1) < 0){
		perror("semop signal_to");
		exit(1);
	}

	return 0;
}

void kill_barber(int signum){
	fprintf(stderr,"Somebody Killed the Barber\n");
	exit(1);
}