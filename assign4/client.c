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
#include <errno.h>

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

int set_queues();
int get_chatID();
int update_list();
int choose_client();
int send_message();
int recv_message();
int hash(int);

int c2s_id,s2c_id;
char chat_id[MAX_BUFFER];
char client_list[MAX_CLIENTS][MAX_BUFFER];
int client_count;
char talking_peer[MAX_BUFFER];
flag skip_send;
flag exit_flag;

int main(){

	client_count = 0;
	fprintf(stdout, "Local Chat Client 1.0\n");
	fflush(stdout);
	set_queues();
	get_chatID();
	exit_flag = FALSE;
	while(!exit_flag){
		update_list();
		skip_send = FALSE;
		choose_client();
		if(!skip_send && !exit_flag)
			send_message();
		recv_message();
	}
	return 0;
}

int set_queues(){

	if ((c2s_id = msgget(KEY_C2S,0660)) < 0)
	{
		perror("Creating Msg Queue client to server");
		exit(1);
	}
	if ((s2c_id = msgget(KEY_S2C, 0660)) < 0)
	{
		perror("Creating Msq Queue server to client");
		exit(1);
	}

	return 0;
}

int get_chatID(){

	message_buffer chat_id_submit;

	fprintf(stdout,"Enter ur chat id:");
	fscanf(stdin,"%10s",chat_id);
	fflush(stdout);

	chat_id_submit.mtype = NEW;
	sprintf(chat_id_submit.mtext,"%s",chat_id);
	if(msgsnd(c2s_id,&chat_id_submit,MAX_BUFFER,IPC_NOWAIT) < 0){
		perror("Sending Chat ID");
		exit(1);
	}
	return 0;
}

int update_list(){

	char list_string[MAX_BUFFER];
	char* token;
	message_buffer list_received;
	while(1){
	if(msgrcv(s2c_id,&list_received,MAX_CLIENTS*MAX_BUFFER,getpid(),IPC_NOWAIT) < 0){
		if(errno == ENOMSG)
			return 0;
		perror("Receiving Client List");
		exit(1);
	}

	client_count = 0;
	strcpy(list_string,list_received.mtext);
	token = strtok(list_string,SPACES);
	while(token != NULL){
		strcpy(client_list[client_count],token);
		client_count++;
		token = strtok(NULL,SPACES);
	}
	}
	return 0;
}

int choose_client(){

	int i,choice;
	flag chosen;

	chosen = FALSE;
	fprintf(stdout, "-1 -> Exit Client\n");
	fprintf(stdout, "0 -> Skip Sending Message\n");
	while(!chosen){
		for(i = 0;i<client_count;i++)
			fprintf(stdout, "%d -> %s\n", i+1,client_list[i]);
		fprintf(stdout, "Choose client:");
		fscanf(stdin,"%4d",&choice);
		if(choice > client_count || choice < -1){
			fprintf(stderr, "Bad Choice");
		}
		else
			chosen = TRUE;

	}
	if(choice == -1)
		exit_flag = TRUE;
	if(choice == 0)
		skip_send = TRUE;
	else
		strcpy(talking_peer,client_list[choice-1]);
	return 0;
}

int send_message(){

	message_buffer sent_msg;
	char msg[MAX_BUFFER];

	fprintf(stdout,"Message to be sent:");
	fscanf(stdin,"%99s",msg);
	sprintf(sent_msg.mtext,"%s %s",talking_peer,msg);

	sent_msg.mtype = MSG;
	if(msgsnd(c2s_id,&sent_msg,MAX_BUFFER,IPC_NOWAIT) < 0){
		perror("Sending Message");
		exit(1);
	}
	return 0;
}

int recv_message(){

	message_buffer msg_recieved;
	fprintf(stdout, "\n" );
	while(1){
		if(msgrcv(s2c_id,&msg_recieved,MAX_BUFFER,hash(getpid()),IPC_NOWAIT) < 0){
			if(errno == ENOMSG)
				return 0;
			perror("Receiving Message");
			exit(1);
		}
		fprintf(stdout,"%s\n\n",msg_recieved.mtext);
		fflush(stdout);	
	}
	return 0;
}

int hash(int pid){
	return pid%1000;
}