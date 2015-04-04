#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_SIZE 4096
#define STR_BUFF 20
#define MSG 11011
#define TRUE (1==1)
#define FALSE (!TRUE)
#define SERVER_FILE "./ser.txt"
#define MSG_MQ_KEY 1001
#define PID_SHM_KEY 1002
#define MSG_SHM_KEY 1003
#define PID_SEM_KEY 1004
#define MSG_SEM_KEY 1005
#define PID_SIZE 500/sizeof(char)
#define MSG_SIZE 5000/sizeof(char)
#define MAX_CLIENTS 10

struct msgbuf{
	long mtype;
	char mtext[MSG_SIZE];
};
typedef int flag;
typedef int ipc_id;
typedef struct {
	int count;
	pid_t array[MAX_CLIENTS];
}pid_array;

int access_IPC_strucures();
int register_client();
int receive_messages();
void input_message(int);
int send_message(char*,char*);
int deregister_client();
int shutdown_server(char*);
int wait_on(ipc_id);
int signal_to(ipc_id);

flag exit_flag;
FILE* server;
ipc_id msg_mq;
ipc_id pid_shm;
ipc_id msg_shm;
ipc_id pid_sem;
ipc_id msg_sem;

char input[MSG_SIZE];
pid_t receiver;
flag is_last;

int main(){

	char* terminal_name;
	terminal_name=ttyname(STDIN_FILENO);
	fprintf(stderr, "Device %s (pid: %d)\n",terminal_name,getpid());

	access_IPC_strucures();

	receiver = fork();
	if(receiver < 0){
		perror("receiver forked");
		exit(1);
	}
	else if(receiver == 0){
		signal(SIGINT, SIG_IGN);
		signal(SIGUSR1, exit);
		register_client();
		receive_messages();
		exit(1);
	}
	else{
		char* login_name;
		
		signal(SIGINT,input_message);
		strcpy(input,".");

		while(1){

			char message[MSG_SIZE];

			strcpy(message,input);
			strcpy(input,".");
			if((login_name = getlogin()) <  0){
				perror("get_login");
				exit(1);
			}
			is_last = FALSE;
			if(strcmp(message,"bye") != 0)
				send_message(login_name,message);
			else
				break;
		}
		deregister_client();
		if(is_last)
			shutdown_server(login_name);
		return 0;
	}
}

int access_IPC_strucures(){

	key_t key;

	if( access( SERVER_FILE, F_OK ) == -1 ) {
		perror(SERVER_FILE);
		exit(1);
	}

	if((key = ftok(SERVER_FILE,MSG_MQ_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((msg_mq = msgget(key,0666)) < 0){
		perror("msg_mq created");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,PID_SHM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((pid_shm = shmget(key,PID_SIZE,0666)) < 0){
		perror("pid_shm created");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,MSG_SHM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((msg_shm = shmget(key,MSG_SIZE,0666)) < 0){
		perror("msg_shm created");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,PID_SEM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((pid_sem = semget(key,1,0666)) < 0){
		perror("pid_sem created");
		exit(1);
	}	

	if((key = ftok(SERVER_FILE,MSG_SEM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((msg_sem = semget(key,1,0666)) < 0){
		perror("msg_sem created");
		exit(1);
	}

	fprintf(stdout,"--- Connected to the server\n");
	return 0;
}

int register_client(){

	wait_on(pid_sem);{
		void* data;
		pid_array* client_list;

		if((data = shmat(pid_shm,NULL,0)) < 0){
			perror("shmatting the pid_shm");
			exit(1);
		}
		client_list =(pid_array*)data;
		client_list->array[client_list->count] = getppid();
		client_list->count++;
		if(shmdt(data) < 0){
			perror("shmdtting the pid_shm");
			exit(1);
		}
	}
	fprintf(stdout,"--- Registered on the server\n");
	fflush(stdout);
	signal_to(pid_sem);
	return 0;
}

int receive_messages(){
	while(1){
		errno = 0;
		struct msgbuf message_buffer;
		if((msgrcv(msg_mq,&message_buffer,sizeof(message_buffer.mtext),getppid(),IPC_NOWAIT)) < 0){
			if(errno == ENOMSG)
				continue;
			perror("message received");
			exit(1);
		}
		fprintf(stdout, "--- Received Message:%s\n",message_buffer.mtext);
		fflush(stdout);
	}
	return 0;
}

void input_message(int signum){

	char format[STR_BUFF];

	fprintf(stdout,"\b\b<Ctrl+C pressed>	\n");
	fprintf(stdout, "--- Enter your message:");
	fflush(stdout);
	sprintf(format,"%%%lds",MSG_SIZE);
	fscanf(stdin,format,input);
	if(strcmp(input,"\n") == 0){
		sprintf(input,".");
	}
}

int send_message(char* login_name,char *message){

	wait_on(msg_sem);{
		char complete_message[MSG_SIZE+STR_BUFF];
		void* data;

		sprintf(complete_message,"%s/%d:%s",login_name,getpid(),message);
		if((data = shmat(msg_shm,NULL,0)) < 0){
			perror("shmatting the msg_shm");
			exit(1);
		}
		sprintf(data,"%s",complete_message);
		if(shmdt(data) < 0){
			perror("shmdtting the msg_shm");
			exit(1);
		}
	}
	signal_to(msg_sem);
	return 0;
}

int deregister_client(){

	wait_on(pid_sem);{
		int i;
		void* data;
		pid_array* client_list;

		if((data = shmat(pid_shm,NULL,0)) < 0){
			perror("shmatting the pid_shm");
			exit(1);
		}
		client_list =(pid_array*)data;
		for(i = 0; i < client_list->count;i++){
			if(client_list->array[i] == getpid()){
				client_list->array[i]=client_list->array[client_list->count-1];
				client_list->count--;
				break;
			}
		}
		if(client_list->count == 0)
			is_last = TRUE;
		if(shmdt(data) < 0){
			perror("shmdtting pid_shm");
			exit(1);
		}
	}
	signal_to(pid_sem);
	kill(receiver,SIGUSR1);
	return 0;
}

int shutdown_server(char* login_name){
	wait_on(msg_sem);{

		char complete_message[MSG_SIZE+STR_BUFF];
		void* data;
		sprintf(complete_message,"%s/%d:*",login_name,getpid());
		if((data = shmat(msg_shm,NULL,0)) < 0){
			perror("shmatting the msg_shm");
			exit(1);
		}
		sprintf(data,"%s",complete_message);
		if(shmdt(data) < 0){
			perror("shmdtting the msg_shm");
			exit(1);
		}
	}
	signal_to(msg_sem);
	return 0;
}

int wait_on(ipc_id sem){


	if(sem == msg_sem){
		struct sembuf operations[2];

		operations[0].sem_num = 0;
		operations[0].sem_op = 0;
		operations[0].sem_flg = 0;

		operations[1].sem_num = 0;
		operations[1].sem_op = 1;
		operations[1].sem_flg = 0;

		while(1){
			errno = 0;
			if((semop(msg_sem,operations,2)) == -1){
				if(errno == EINTR)
					continue;
				perror("wait_msg_sem");
				exit(1);
			}
			break;
		}
	}
	else if(sem == pid_sem){
		struct sembuf operations[1];

		operations[0].sem_num = 0;
		operations[0].sem_op = -1;
		operations[0].sem_flg = 0;

		while(1){
			errno = 0;
			if((semop(pid_sem,operations,1)) == -1){
				if(errno == EINTR)
					continue;
				perror("wait_pid_sem");
				exit(1);
			}
			break;
		}	
	}
	return 0;
}

int signal_to(ipc_id sem){

	if(sem == msg_sem){
		struct sembuf operations[1];

		operations[0].sem_num = 0;
		operations[0].sem_op = 1;
		operations[0].sem_flg = 0;

		while(1){
			errno = 0;
			if((semop(msg_sem,operations,1)) == -1){
				if(errno == EINTR)
					continue;
				perror("signal_msg_sem");
				exit(1);
			}
			break;
		}
	}
	else if(sem == pid_sem){
		struct sembuf operations[1];

		operations[0].sem_num = 0;
		operations[0].sem_op = 1;
		operations[0].sem_flg = 0;

		while(1){
			errno = 0;
			if((semop(pid_sem,operations,1)) == -1){
				if(errno == EINTR)
					continue;
				perror("signal_pid_sem");
				exit(1);
			}
			break;
		}	
	}
	return 0;
}