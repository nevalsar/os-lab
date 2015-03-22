#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>

//include "shop.h"

#define BARBER_KEY 1000
#define CUSTOMER_KEY 1001
#define MUTEX_KEY 1002
#define SEAT_STAT_KEY 1003
#define STR_BFFR 50

int access_semaphores();
int access_queues();
int get_free_seats();
int wait_on(int);
int signal_to(int);

typedef struct msgbuf {
	long    mtype;
	char    mtext[STR_BFFR];
}message_buffer;

typedef struct sembuf sem_buffer;

int customers_waiting;
int barber_idle;
int mutex;
int seat_status;
int customer_id;
pid_t start;

int main(int argc,char** argv){

	int free_seats;
	customer_id = atoi(argv[1]);
	start = atoi(argv[2]);
	access_semaphores();
	access_queues();
	fprintf(stderr, "Customer %d:Trying to access waiting room chairs\n",customer_id );
	wait_on(mutex);
	kill(start,SIGPIPE);
	get_free_seats(&free_seats);
	if(free_seats > 0){
		kill(start,SIGUSR1);
		fprintf(stderr, "Customer %d:Acquired a chair\n" ,customer_id);
		fprintf(stderr, "Customer %d:Notifying the barber\n",customer_id);
		signal_to(customers_waiting);
		fprintf(stderr, "Customer %d:Release control over the access to waiting room chairs\n",customer_id);
		signal_to(mutex);
		fprintf(stderr, "Customer %d:Waiting until barber is ready\n",customer_id);
		wait_on(barber_idle);
		fprintf(stderr, "Customer %d:Getting Haircut\n",customer_id);
		//gethaircut
	}
	else{
		fprintf(stderr, "Customer %d:No free chair\n",customer_id);
		fprintf(stderr, "Customer %d:Leaving without haircut\n",customer_id);
		signal_to(mutex);
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

int access_queues(){

	if ((seat_status = msgget(SEAT_STAT_KEY, 0660)) < 0)
	{
		perror("Accessing Msq Queue");
		exit(1);
	}

	return 0;
}

int get_free_seats(int* free_seats){

	message_buffer list_received;

	if(msgrcv(seat_status,&list_received,STR_BFFR,1,0) < 0){
		perror("Receiving Free Seats");
		exit(1);
	}
	sscanf(list_received.mtext,"%2d",free_seats);
	fprintf(stderr, "Customer %d:I see %d free_Seats\n",customer_id,*free_seats);
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
		perror("semop wait_on");
		exit(1);
	}
	return 0;
}

void kill_barber(){

}