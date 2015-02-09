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

#define CHILD_COUNT 5
#define N 25;
#define BUFFER 20

typedef int flag;

int create_children();
int allot_ids();
int select_pivot();
int get_larger_count();
int remove_larger();
int remove_smaller();
int terminate();

FILE* child_out[CHILD_COUNT];
int child_in[CHILD_COUNT];

int pivot,larger_count,median;
int c2p_fds[CHILD_COUNT][2];
int p2c_fds[CHILD_COUNT][2];
pid_t children[CHILD_COUNT];

int main(){

	median = N;
	median /= 2;
	srand (time(NULL));;
	
	create_children();
	allot_ids();
	while(1){
		larger_count = 0;
		select_pivot();
		get_larger_count();
		if(larger_count == median){
			fprintf(stdout,"--Parent %d numbers more than pivot %d . Median Found !!\n",larger_count,pivot);
			break;
		}
		else if(larger_count > median){
			remove_smaller();
			sleep(1);
		}
		else{
			remove_larger();
			sleep(1);
			median -= larger_count;
		}
	}
	terminate();

	return 0;
}

int create_children(){

	int i;

	for(i = 0; i < CHILD_COUNT; i++){
		int status;
		status = pipe(c2p_fds[i]);
		if(status == -1)
			perror("pipe:child to parent:");
		status = pipe(p2c_fds[i]);
		if(status == -1)
			perror("pipe:child to parent:");
		
		
	}
	for(i = 0;i < CHILD_COUNT; i++){
		children[i] = fork();
		if(children[i] == 0){
			char* args[4];

			close(c2p_fds[i][0]);
			close(p2c_fds[i][1]);

			args[0] = (char*)malloc(sizeof(char)*BUFFER);
			sprintf(args[0],"./child.out");
			args[1] = (char*)malloc(sizeof(char)*BUFFER);
			sprintf(args[1],"%d",p2c_fds[i][0]);
			args[2] = (char*)malloc(sizeof(char)*BUFFER);
			sprintf(args[2],"%d",c2p_fds[i][1]);
			args[3] = NULL	;
			
			execvp(args[0],args);
			perror("execvp");
			_exit(1);

		}
		else if(children[i] > 0){
			close(c2p_fds[i][1]);
			close(p2c_fds[i][0]);

			child_in[i] = c2p_fds[i][0];
			child_out[i] = fdopen(p2c_fds[i][1],"w");

		}
		else
			perror("fork");

	}
	return 0;
} 

int allot_ids(){

	flag ready;
	int i;

	for(i = 0;i < CHILD_COUNT; i++){
		fprintf(child_out[i],"%d",i);
		fflush(child_out[i]);	
	}
	fprintf(stderr, "data sent\n" );
	for(i = 0;i < CHILD_COUNT; i++){
		ready = 0;
		while(ready != READY){
			char readbuffer[BUFFER];
			read(child_in[i],readbuffer,sizeof(readbuffer));
			ready = atoi(readbuffer);
		}
		
	}
	fprintf(stdout, "--Parent READY\n");
	return 0;
}

int select_pivot(){

	int i;

	pivot = -1;
	while( pivot == -1){
		i = rand()%CHILD_COUNT;
		fprintf(stdout,"--Parent sends REQUEST to Child %d\n",i+1);
		fprintf(child_out[i], "%d", REQUEST);
		fflush(child_out[i]);
		char readbuffer[BUFFER];
		memset(&readbuffer[0], 0, sizeof(readbuffer));
		read(child_in[i],readbuffer,sizeof(readbuffer));
		pivot = atoi(readbuffer);
	}
	fprintf(stdout,"--Parent broadcasts pivot %d to all children\n",pivot);
	for(i = 0;i < CHILD_COUNT; i++){
		fprintf(child_out[i],"%d",PIVOT);
		fflush(child_out[i]);
		sleep(1);
		fprintf(child_out[i],"%d",pivot);
		fflush(child_out[i]);
		
	}
	return 0;
}

int get_larger_count(){

	int i,larger_count_child;
	larger_count = 0;
	for(i = 0;i < CHILD_COUNT; i++){
		char readbuffer[BUFFER];
		memset(&readbuffer[0], 0, sizeof(readbuffer));
		read(child_in[i],readbuffer,sizeof(readbuffer));
		larger_count_child = atoi(readbuffer);
		larger_count += larger_count_child;
		fprintf(stdout, "--Child %d receives pivot and replies %d\n",i+1,larger_count_child);
	}
	
	return 0;
}

int remove_larger(){

	int i;
	for(i = 0;i < CHILD_COUNT; i++){
		fprintf(child_out[i],"%d",LARGE);
		fflush(child_out[i]);
	}
	return 0;
}

int remove_smaller(){

	int i;
	for(i = 0;i < CHILD_COUNT; i++){
		fprintf(child_out[i],"%d",SMALL);
		fflush(child_out[i]);
	}
	return 0;
}

int terminate(){

	int i;
	fprintf(stdout,"--Parent sends kill signals to all children\n");
	for(i = 0;i < CHILD_COUNT; i++){
		kill(children[i],SIGUSR1);
		fclose(child_out[i]);
		close(child_in[i]);
	}
	return 0;

}