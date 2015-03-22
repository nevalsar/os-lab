#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>

//#include "shop.h"

#define BARBER_KEY 1000
#define CUSTOMER_KEY 1001
#define MUTEX_KEY 1002
#define SEAT_STAT_KEY 1003
#define STR_BFFR 50
#define MAX_CUSTOMERS 4
#define INCREASE

int create_semaphores();
int create_msg_queues();
int create_barber();
int open_shop();
void occupy_seat(int);
void vacate_seat(int);
void is_empty_seat_available(int);
void close_shop(int);

typedef struct msgbuf {
	long    mtype;
	char    mtext[STR_BFFR];
}message_buffer;

typedef union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
} sem_union;

typedef struct sembuf sem_buffer;

int customers_waiting;
int barber_idle;
int mutex;
int shop_barber;
int seat_status;
int chair_count;
int free_seats;
int customer_count;
pid_t barber;
pid_t customers[MAX_CUSTOMERS];

int main(){
	signal(SIGINT,close_shop);
	signal(SIGUSR1,occupy_seat);
	signal(SIGUSR2,vacate_seat);
	signal(SIGPIPE,is_empty_seat_available);
	create_semaphores();
	create_msg_queues();
	create_barber();
	sleep(1);
	fprintf(stdout,"No. of chairs in shop:");
	fscanf(stdin,"%4d",&chair_count);
	free_seats = chair_count;
	open_shop();
	//while(1);
	close_shop(0);
	return 0;
}

int create_semaphores(){
	sem_union initiliaser_customers_waiting;
	sem_union initiliaser_barber_idle;
	sem_union initiliaser_mutex;
	unsigned short values_customers_waiting[1];
	unsigned short values_barber_idle[1];
	unsigned short values_mutex[1];
	if((customers_waiting = semget(CUSTOMER_KEY,1,IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("Semaphore customers created");
		exit(1);
	}
	if((barber_idle = semget(BARBER_KEY,1,IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("Semaphore barber created");
		exit(1);
	}
	if((mutex = semget(MUTEX_KEY,1,IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("Semaphore mutex created");
		exit(1);
	}
	values_customers_waiting[0] = 0;
	values_barber_idle[0] = 0;
	values_mutex[0] = 1;
	initiliaser_customers_waiting.array = values_customers_waiting;
	initiliaser_barber_idle.array = values_barber_idle;
	initiliaser_mutex.array = values_mutex;

	if(semctl(customers_waiting,0,SETALL,initiliaser_customers_waiting)< 0){
		perror("Semaphore customers initialised");
		exit(1);
	}

	if(semctl(barber_idle,0,SETALL,initiliaser_barber_idle)< 0){
		perror("Semaphore barber initialised");
		exit(1);
	}

	if(semctl(mutex,0,SETALL,initiliaser_mutex)< 0){
		perror("Semaphore mutex initialised");
		exit(1);
	}
	return 0;
}

int create_msg_queues(){
	if ((seat_status = msgget(SEAT_STAT_KEY,IPC_CREAT | IPC_EXCL | 0660)) < 0)
	{
		perror("Creating Msg Queue");
		exit(1);
	}
	return 0;
}

int create_barber(){
	barber = fork();
	if(barber < 0){
		perror("Barber created");
		exit(1);
	}
	else if(barber == 0){
		char* args[3];
		args[0] = (char*) malloc(sizeof(char)*STR_BFFR);
		strcpy(args[0],"./barber");
		args[1] = (char*) malloc(sizeof(char)*STR_BFFR);
		sprintf(args[1],"%d",getppid());
		args[2] = (char*) malloc(sizeof(char)*STR_BFFR);
		strcpy(args[2],"\0");
		execvp(args[0],args);
		perror("Barber exec'd");
		exit(1);
	}

	return 0;
}

int open_shop(){

	int i;
	customer_count = 0;
	while(customer_count < MAX_CUSTOMERS){
		
		customers[customer_count] = fork();
		if(customers[customer_count] < 0){
			perror("New Customer");
			exit(1);
		}
		else if(customers[customer_count] == 0){
			char* args[4];
			args[0] = (char*) malloc(sizeof(char)*STR_BFFR);
			strcpy(args[0],"./customer");
			args[1] = (char*) malloc(sizeof(char)*STR_BFFR);
			sprintf(args[1],"%d",customer_count);
			args[2] = (char*) malloc(sizeof(char)*STR_BFFR);
			sprintf(args[2],"%d",getppid());
			args[3] = (char*) malloc(sizeof(char)*STR_BFFR);
			strcpy(args[3],"\0");
			execvp(args[0],args);
			perror("Customers exec'd");
			exit(1);
		}
		else
			customer_count++;
	}
	for(i = 0;i < MAX_CUSTOMERS;i++)
		waitpid(customers[i],NULL,0);

	return 0;
}

void occupy_seat(int signum){
	free_seats--;
}

void vacate_seat(int signum){
	free_seats++;
}

void is_empty_seat_available(int signum){
	message_buffer snd_msg;

	sprintf(snd_msg.mtext,"%d",free_seats);
	snd_msg.mtype = 1;
	if(msgsnd(seat_status,&snd_msg,STR_BFFR,IPC_NOWAIT) < 0){
			perror("Forwarding the message to destination");
			exit(1);
	}
}

void close_shop(int signum){
	kill(barber,SIGINT);
	fprintf(stderr, "Havoc!! Destroy Semaphores and Message-Queues\n" );
	if(semctl(customers_waiting,0,IPC_RMID,0) < 0){
		perror("Semaphore destroyed");
		exit(1);
	}
	if(semctl(barber_idle,0,IPC_RMID,0) < 0){
		perror("Semaphore destroyed");
		exit(1);
	}
	if(semctl(mutex,0,IPC_RMID,0) < 0){
		perror("Semaphore destroyed");
		exit(1);
	}
	if(msgctl(seat_status,IPC_RMID,0) < 0){
		perror("Removing Queue Filed");
		exit(1);
	}
	exit(0);
}