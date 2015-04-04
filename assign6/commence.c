#include <stdio.h>
#include <stdlib.h>
int main(){
	if(system("clear") < 0){
		perror("clear");
		exit(0);
	}
	fprintf(stdout,"COMMENCING CONFERENCE: RESPOND\n" );
	exit(0);
}