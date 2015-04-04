#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
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
#include <unistd.h>
#include <utmp.h>

#define MAX_SIZE 4096
#define STR_BUFF 20
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
	char mtext[STR_BUFF];
};
typedef int flag;
typedef int ipc_id;
typedef struct {
	int count;
	pid_t array[MAX_CLIENTS];
}pid_array;
union semun {
	int val;               
	struct semid_ds *buf;  
	ushort *array;         
};

int create_IPC_structures();
int identify_users();
int user_allowed(char*,char**,int);
int receive_message(char*);
int broadcast_messages(char*,int);
int wait_on(ipc_id);
int signal_to(ipc_id);
void terminate(int);

FILE* server;
ipc_id msg_mq;
ipc_id pid_shm;
ipc_id msg_shm;
ipc_id pid_sem;
ipc_id msg_sem;

int main(int argc,char** argv){


	flag exit_flag;
	signal(SIGSEGV,terminate);
	signal(SIGINT,terminate);
	signal(SIGABRT,terminate);
	exit_flag = FALSE;
	create_IPC_structures();
	identify_users(argv,argc);
	while(!exit_flag){
		wait_on(msg_sem);{
			char signed_message[MSG_SIZE+STR_BUFF];
			char format[STR_BUFF];
			char message[MSG_SIZE];
			int client_pid;
			char login[STR_BUFF];
	
			receive_message(signed_message);

			sprintf(format, "%%%d[^/]/%%d:%%%lds",STR_BUFF,MSG_SIZE);
			sscanf(signed_message,format,login,&client_pid,message);

			if(strcmp(message,"*") == 0)
				exit_flag = TRUE;
			else if(strcmp(message,".") != 0){
				fprintf(stdout,"--- Received msg \"%s\"\n",signed_message);
				broadcast_messages(signed_message,client_pid);
			}
		}
		signal_to(msg_sem);
	}
	terminate(0);
	return 0;
}

int create_IPC_structures(){

	key_t key;
	union semun sem_init;

	if((server = fopen(SERVER_FILE,"wx")) == NULL){
		perror(SERVER_FILE);
		exit(1);
	}

	fprintf(server,"%d",getpid());
	if((key = ftok(SERVER_FILE,MSG_MQ_KEY)) == -1){
		perror("key34 acquired");
		exit(1);
	}
	if((msg_mq = msgget(key,IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("msg_mq created");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,PID_SHM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((pid_shm = shmget(key,PID_SIZE*sizeof(char),IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("pid_shm created");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,MSG_SHM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((msg_shm = shmget(key,MSG_SIZE*sizeof(char),IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("msg_shm created");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,PID_SEM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}
	if((pid_sem = semget(key,1,IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("pid_sem created");
		exit(1);
	}	

	sem_init.val = 1;
	if((semctl(pid_sem,0,SETVAL,sem_init)) < 0){
		perror("pid_sem initialised");
		exit(1);
	}

	if((key = ftok(SERVER_FILE,MSG_SEM_KEY)) == -1){
		perror("key acquired");
		exit(1);
	}

	if((msg_sem = semget(key,1,IPC_CREAT | IPC_EXCL | 0666)) < 0){
		perror("msg_sem created");
		exit(1);
	}

	sem_init.val = 0;
	if((semctl(msg_sem,0,SETVAL,sem_init)) < 0){
		perror("msg_sem_initialised");
		exit(1);
	}

	wait_on(pid_sem);{
		void* data;
		if((data = shmat(pid_shm,NULL,0)) < 0){
			perror("shmatting the pid_shm");
			exit(1);
		}
		((pid_array*)data)->count = 0;
		if(shmdt(data) < 0){
			perror("shmdtting pid_shm");
			exit(1);
		}
	}
	signal_to(pid_sem);

	fprintf(stdout,"--- Initialisation Complete\n");
	return 0;
}

int identify_users(char **users,int user_length){

	struct utmp log[MAX_SIZE];
	FILE* utmp_file;

	if((utmp_file = fopen("/var/run/utmp","rb")) < 0 ){
		perror("utmp");
		exit(1);
	}
	if(fread(&log,sizeof(struct utmp),sizeof(log),utmp_file) < 0){
		perror("utmp");
		exit(1);
	}
	int i;
	for(i = 0; i < sizeof(log)/sizeof(struct utmp); i++){
		
		if(getpwnam(log[i].ut_user) != NULL){
			char terminal_id[STR_BUFF];
			sprintf(terminal_id, "/dev/%s",log[i].ut_line);
			if(strcmp(terminal_id,ttyname(STDIN_FILENO)) != 0 && strcmp(log[i].ut_user,getlogin()) == 0){
				if(user_allowed(log[i].ut_user,users,user_length)){
					char invitation[STR_BUFF];
					fprintf(stdout, "--- Sending commence notification to %s\n",terminal_id);
					sprintf(invitation, "./commence > %s", terminal_id);
					if(system(invitation) < 0){
						perror("system");
						exit(1);
					}
				}
			}
		}
	}
	fclose(utmp_file);
	return 0;
}

int user_allowed(char* key,char** list,int length){

	int i;
	for(i = 0; i < length; i++){
		if(strcmp(key,list[i]) == 0)
			return TRUE;
	}
	return FALSE;
}

int receive_message(char* signed_message){

	void* data;

	if((data = shmat(msg_shm,NULL,0)) < 0){
		perror("shmatting the msg_shm");
		exit(1);
	}
	strcpy(signed_message,data);
	if(shmdt(data) < 0){
		perror("shmdtting the msg_shm");
		exit(1);
	}
	return 0;
}

int broadcast_messages(char* signed_message,int client_pid){

	wait_on(pid_sem);{
		int i;	
		void* data;
		pid_array* client_list;
		if((data = shmat(pid_shm,NULL,0)) < 0){
			perror("shmatting the pid_shm");
			exit(1);
		}
		client_list = (pid_array*)data;
		struct msgbuf message_buffer[client_list->count];

		for( i = 0; i < client_list->count; i++){
			if(client_list->array[i] != client_pid){
				strcpy(message_buffer[i].mtext,signed_message);
				message_buffer[i].mtype = client_list->array[i];
				if((msgsnd(msg_mq,&message_buffer[i],sizeof(message_buffer[i]),IPC_NOWAIT) ) < 0){
					perror("message broadcasted");
					exit(1);
				}
				fprintf(stdout, "--- Sending msg to %ld\n",message_buffer[i].mtype);
			}
		}
		if(shmdt(data) < 0){
			perror("shmdtting the pid_shm");
			exit(1);
		}	
	}
	signal_to(pid_sem);
	return 0;
}

int wait_on(ipc_id sem){

	if(sem == msg_sem){
		struct sembuf operations[2];

		operations[0].sem_num = 0;
		operations[0].sem_op = -2;
		operations[0].sem_flg = 0;

		operations[1].sem_num = 0;
		operations[1].sem_op = 1;
		operations[1].sem_flg = 0;

		while(1){
			errno = 0;
			if((semop(msg_sem,operations,2))< 0){
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
			if((semop(pid_sem,operations,1)) < 0){
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
		operations[0].sem_op = -1;
		operations[0].sem_flg = 0;

		while(1){
			errno = 0;
			if((semop(msg_sem,operations,1)) < 0){
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
			if((semop(pid_sem,operations,1)) < 0){
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

void terminate(int signum){

	if(msgctl(msg_mq,IPC_RMID,NULL) < 0){
		perror("msg_mq removed");
	}

	if(shmctl(pid_shm,IPC_RMID,NULL) < 0){
		perror("pid_shm removed");
	}	

	if(shmctl(msg_shm,IPC_RMID,NULL) < 0){
		perror("msg_shm removed");
	}

	if(semctl(msg_sem,0,IPC_RMID,NULL) < 0){
		perror("pid_sem removed");
	}

	if(semctl(pid_sem,0,IPC_RMID,NULL) < 0){
		perror("msg_sem removed");
	}
	fclose(server);
	char cmd[STR_BUFF];
	strcpy(cmd,"rm -rf ");
	strcat(cmd,SERVER_FILE);
	system(cmd);
	fprintf(stdout, "\b\b--- Terminating conference\n");
	exit(0);
}