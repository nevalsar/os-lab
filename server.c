/*
	PRANJAL PANDEY 12CS30026
	NEVIN VALSARAJ 12CS10032
	Assignment 4
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define KEY_C2S 1234
#define KEY_S2C 1235
#define NEW 100
#define MSG 102

#define SPACES " \t\r\n"
#define TRUE (1==1)
#define FALSE (!TRUE)

#define MAX_CLIENTS 10
#define MAX_BUFFER 100

typedef int flag;
typedef struct msgbuf {
	long    mtype;
	char    mtext[MAX_BUFFER*MAX_CLIENTS];
}message_buffer;
typedef struct msqid_ds statistics_buffer;

int set_queues();
int update_mapping();
int send_list();
int forward_message();
int search_pid(char*,int*);
int search_chat_id(int,char*);
int hash(int);
void terminate(int);

int c2s_id,s2c_id;
char* chat_id;
pid_t client_pid_list[MAX_CLIENTS];
char client_chat_list[MAX_CLIENTS][MAX_BUFFER];
int client_count;
char* talking_peer;
flag skip_send;
flag exit_flag;

int main(){

	client_count = 0;
	signal(SIGINT,terminate);
	signal(SIGTERM,terminate);

	fprintf(stdout, "Local Chat Server 1.0\n Use Ctrl-C to Exit\n");
	fflush(stdout);
	set_queues();
	exit_flag = FALSE;
	while(!exit_flag){
		skip_send = FALSE;
		update_mapping();
		if(!skip_send){
			send_list();
		}
		forward_message();
	}
	return 0;
}

int set_queues(){

	if ((c2s_id = msgget(KEY_C2S,IPC_CREAT | IPC_EXCL | 0660)) < 0)
	{
		perror("Creating Msg Queue client to server");
		exit(1);
	}
	if ((s2c_id = msgget(KEY_S2C,IPC_CREAT | IPC_EXCL | 0660)) < 0)
	{
		perror("Creating Msq Queue server to client");
		exit(1);
	}

	return 0;
}

int update_mapping(){

	message_buffer new_client;
	statistics_buffer stats;
	skip_send = TRUE;
	while(1){
		if(msgrcv(c2s_id,&new_client,MAX_BUFFER,NEW,IPC_NOWAIT) < 0){
			if(errno == ENOMSG)
				return 0;
			perror("Receiving New Client");
			exit(1);
		}
		skip_send = FALSE;
		if(msgctl(c2s_id,IPC_STAT,&stats) < 0){
			perror("Getting New Client PID");
			exit(1);
		}
		// fprintf(stdout,"NEW CLIENT:%s PID:%d CLIENTS:%d\n",new_client.mtext,stats.msg_lspid,client_count);
		sprintf(client_chat_list[client_count],"%s",new_client.mtext);
		client_pid_list[client_count]=stats.msg_lspid;
		client_count++;
		fprintf(stdout,"NEW CLIENT:%s PID:%d CLIENTS:%d\n",client_chat_list[client_count-1],client_pid_list[client_count-1],client_count);
		fflush(stdout);
	}
	return 0;
}

int send_list(){

	message_buffer chat_id_list;
	int i;
	strcpy(chat_id_list.mtext,"");
	for(i=0; i< client_count-1; i++){
		strcat(chat_id_list.mtext,client_chat_list[i]);
		strcat(chat_id_list.mtext," ");
	}
	strcat(chat_id_list.mtext,client_chat_list[i]);
	

	for(i =0;i<client_count;i++){
		chat_id_list.mtype = client_pid_list[i];
		if(msgsnd(s2c_id,&chat_id_list,MAX_BUFFER*MAX_CLIENTS,IPC_NOWAIT) < 0){
			perror("Sending List of Chat ID");
			exit(1);
		}
	}

	return 0;
}

int forward_message(){

	message_buffer recv_msg;
	message_buffer snd_msg;
	statistics_buffer stats,stats1;
	pid_t pid_sender,pid_receiver;
	char chat_id_sender[MAX_BUFFER];
	char chat_id_receiver[MAX_BUFFER];
	char msg[MAX_BUFFER];

	while(1){
		if(msgrcv(c2s_id,&recv_msg,MAX_BUFFER,MSG,IPC_NOWAIT) < 0){
			if(errno == ENOMSG)
				return 0;
			perror("Receiving Message From Source");
			exit(1);
		}



		strncpy(chat_id_receiver,recv_msg.mtext,strcspn(recv_msg.mtext,SPACES));
		chat_id_receiver[strcspn(recv_msg.mtext,SPACES)] = '\0';
		strncpy(msg,&recv_msg.mtext[strcspn(recv_msg.mtext,SPACES)],sizeof(msg));
		search_pid(chat_id_receiver,&pid_receiver);

		if(msgctl(c2s_id,IPC_STAT,&stats) < 0){
			perror("Getting Source Message Stats");
			exit(1);
		}

		if(msgctl(s2c_id,IPC_STAT,&stats1) < 0){
			perror("Getting Source Message Stats");
			exit(1);
		}

		pid_sender = stats.msg_lspid;
		search_chat_id(pid_sender,chat_id_sender);

		fprintf(stdout, "MSG FROM %s TO %s:\n%s\n",chat_id_sender,chat_id_receiver,msg);
		fflush(stdout);
		fprintf(stdout, "Messages in client to server Queue:%d\n",(int)stats.msg_qnum);
		fflush(stdout);
		fprintf(stdout, "Messages in server to client Queue:%d\n",(int)stats1.msg_qnum);
		fflush(stdout);

		sprintf(snd_msg.mtext,"%s ",chat_id_sender);
		strcat(snd_msg.mtext,"<");
		strcat(snd_msg.mtext,ctime(&stats.msg_rtime));
		strcat(snd_msg.mtext,"> ");
		strcat(snd_msg.mtext,msg);
		snd_msg.mtype = hash(pid_receiver);
		if(msgsnd(s2c_id,&snd_msg,MAX_BUFFER,IPC_NOWAIT) < 0){
			perror("Forwarding the message to destination");
			exit(1);
		}
	}
}


int search_pid(char* chat_id,int* pid){

	int i;
	for(i = 0;i<client_count;i++)
		if(strcmp(chat_id,client_chat_list[i]) == 0){
			*pid = client_pid_list[i];
			return 0;
		}
		*pid = -1;
		return 0;
	}

int search_chat_id(int pid, char* chat_id){

	int i;
	for(i = 0;i<client_count;i++)
		if(client_pid_list[i] == pid){
			strcpy(chat_id,client_chat_list[i]);
			return 0;
		}
		return 0;
}

int hash(int pid){
	return pid%1000;
}

void terminate(int signum){

	if(msgctl(c2s_id,IPC_RMID,0) < 0){
			perror("Removing Queue Filed");
			exit(1);
	}
	if(msgctl(s2c_id,IPC_RMID,0) < 0){
			perror("Removing Queue Filed");
			exit(1);
	}
}