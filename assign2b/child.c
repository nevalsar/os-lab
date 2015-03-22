/*==============================
==	PRANJAL PANDEY 12CS30026  ==
==	NEVIN VALSARAJ 12CS10032  ==
==	Assignment 2b			  ==
===============================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define REQUEST 100
#define PIVOT 200
#define LARGE 300
#define SMALL 400
#define READY 500

#define FILE_BUFFER 50

int readInput();
int wait_to_serve();
int get_random_element();
int get_count_larger();
int delete_smaller();
int delete_larger();
void terminate(int);

typedef int flag;

int iden,pivot,data_length;
int* data;

int parent_in;
FILE* parent_out;

int main(int argc, char* argv[]){

	int fdr,fdw;
	srand(time(NULL));

	fdr = atoi(argv[1]);
	fdw = atoi(argv[2]);
	parent_in = fdr;
	parent_out = fdopen(fdw,"w");
	data_length = 0;

	signal(SIGUSR1,terminate);
	signal(SIGUSR2,terminate);
	
	char readbuffer[FILE_BUFFER];
	read(parent_in,readbuffer,sizeof(readbuffer));
	iden = atoi(readbuffer);
	readInput();
	fprintf(stdout,"--Child %d sends READY\n",iden+1);
	fprintf(parent_out,"%d",READY);
	fflush(parent_out);
	fflush(stdout);
	while(1)
		wait_to_serve();

	return 0;
}

int readInput(){

	char filename[FILE_BUFFER];
	FILE* data_in;
	int i;

	i = 0;
	sprintf(filename,"data_%d.txt",iden+1);
	data_in = fopen(filename,"r");
	data = malloc(sizeof(int) * FILE_BUFFER);

	if(data_in == NULL){
		perror(strcat(filename,":"));
		exit(1);
	}

	while (feof (data_in) == 0){

		fscanf(data_in, "%d",&data[i]);
		i++;
	}
	fclose(data_in);
	data_length = i;
	return 0;
}

int wait_to_serve(){


		int code;
		code = 0;
		char readbuffer[FILE_BUFFER];
		memset(&readbuffer[0], 0, sizeof(readbuffer));
		read(parent_in,readbuffer,sizeof(readbuffer));
		code = atoi(readbuffer);
		
		switch(code){
			case REQUEST:get_random_element(); break;
			case PIVOT:get_count_larger();break;
			case SMALL:delete_smaller(); break;
			case LARGE:delete_larger(); break;
			default: break;
		}
	return 0;
}

int get_random_element(){

	if(data_length == 0)
		fprintf(parent_out, "%d", -1);
	else{
		int random_index;
		random_index = rand()%data_length;
		fprintf(parent_out, "%d", data[random_index]);
		fprintf(stdout,"--Child %d sends %d to parent\n",iden+1,data[random_index]);
	}
	fflush(parent_out);
	fflush(stdout);
	return 0;
}

int get_count_larger(){

	char readbuffer[FILE_BUFFER];
	pivot = -1;
	memset(&readbuffer[0], 0, sizeof(readbuffer));
	read(parent_in,readbuffer,sizeof(readbuffer));
	pivot = atoi(readbuffer);
	if(data_length == 0)
		fprintf(parent_out, "%d", 0);
	else{
		int count,i;
	
		count = 0;
		for(i = 0;i < data_length;i++){
			if(data[i] > pivot)
				count++;
		}
		fprintf(parent_out, "%d", count);
	}
	fflush(parent_out);
	return 0;
}

int delete_smaller(){

	int* data_temp;
	int i,j;

	i = j = 0;
	data_temp = data;
	data = malloc(sizeof(int)*FILE_BUFFER);
	for(i = 0;i<data_length;i++){
		if(data_temp[i] >= pivot){
			data[j] = data_temp[i];
			j++;
		}
	}
	data_length = j;
	fprintf(stdout, "--Child %d removes %d smaller elements\n",iden+1,i-j);
	fflush(stdout);
	return 0;
}

int delete_larger(){

	int* data_temp;
	int i,j;
	i = j = 0;
	data_temp = data;
	data = malloc(sizeof(int)*FILE_BUFFER);

	for(i = 0;i<data_length;i++){
		if(data_temp[i] <= pivot){
			data[j] = data_temp[i];
			j++;
		}
	}
	data_length = j;
	fprintf(stdout, "--Child %d removes %d larger elements\n",iden+1,i-j);
	fflush(stdout);
	return 0;
}

void terminate(int signum){
	fprintf(stdout, "-- Child %d terminates\n", iden);
	
	fclose(parent_out);
	close(parent_in);
	exit(signum);
}