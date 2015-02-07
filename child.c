#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "parent.c"

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

FILE* parent_in;
FILE* parent_out;

int main(int argc, char* argv[]){

	int fdr,fdw;

	fdr = atoi(argv[1]);
	fdw = atoi(argv[2]);
	parent_in = fdopen(fdr,"r");
	parent_out = fdopen(fdw,"w");
	data_length = 0;

	signal(SIGUSR1,terminate);
	signal(SIGUSR2,terminate);

	fscanf(parent_in,"%d",&iden);
	readInput();
	fprintf(parent_out,"%d",READY);
	fflush(parent_out);
	fflush(parent_in);
	wait_to_serve();
	return 0;
}

int readInput(){

	char filename[FILE_BUFFER];
	FILE* data_in;
	int i;

	i = 0;
	sprintf(filename,"data_%d.txt",iden);
	data_in = fopen(filename,"r");
	data = malloc(sizeof(int) * FILE_BUFFER);

	if(data_in == NULL){
		perror(strcat(filename,":"));
		exit(1);
	}

	while (feof (data_in) != 0){
		fscanf(data_in, "%d",&data[i]);
		i++;
	}
	fclose(data_in);
	data_length = i;
	
	return 0;
}

int wait_to_serve(){

	int code;
	while(1){
		fscanf(parent_in,"%d",&code);
		switch(code){
			case REQUEST:get_random_element();
			case PIVOT:get_count_larger();
			case SMALL:delete_smaller();
			case LARGE:delete_larger();
			default: ;
		}
	}
}

int get_random_element(){

	if(data_length == 0)
		fprintf(parent_out, "%d", -1);
	else{
		int random_index;
		random_index = rand()/data_length;
		fprintf(parent_out, "%d", random_index);

	}
	fflush(parent_out);
	return 0;
}

int get_count_larger(){

	
	pivot = -1;
	if(data_length == 0)
		fprintf(parent_out, "%d", 0);
	else{
		int count,i;
		count = 0;
		fscanf(parent_in,"%d",&pivot);
		for(i = 0;i<data_length;i++){
			if(data[i] > pivot)
				count++;
		}
		fprintf(parent_out, "%d", count);
		fflush(parent_in);
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

	for(i = 0;i<data_length;i++,j++){
		if(data[i] >= pivot)
			data[j] = data_temp[i];
	}
	data_length = j;
	return 0;
}

int delete_larger(){

	int* data_temp;
	int i,j;

	i = j = 0;
	data_temp = data;
	data = malloc(sizeof(int)*FILE_BUFFER);

	for(i = 0;i<data_length;i++,j++){
		if(data[i] <= pivot)
			data[j] = data_temp[i];
	}
	data_length = 0;
	return 0;
}

void terminate(int signum){
	exit(signum);
}