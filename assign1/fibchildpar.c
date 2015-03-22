/*
Nevin Valsaraj
12CS10032
Pranjal Pandey
12CS30026
Assignment 1 Part 2
Prints a ordered set of 'nfib' fibonacci numbers in the child processes and printing in master process
*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

//CALCULATES iTH FIBONACCI AND "EXITS" WITH THE RESULT AS THE RETURN CODE
int forkedfibo(int i){
  int a,b,c;
  a = b = c = 1;
  if(i == 0 || i == 1)
    _exit(1);

    while(i >= 2){
      c = a + b;
      a = b;
      b = c;
      i--;
    }
    _exit(c);
  }

int main(){
  int nfib,i,status;
  pid_t pid,c_pid;

  pid = getpid();

  scanf("%d",&nfib);
  int fib[nfib];
  pid_t c_pids[nfib];

 //LOOP for nfib times
  for(i=0;i<nfib;i++)
    if(pid != 0){
      pid = fork();             // fork a child process
      if(pid == 0)
        forkedfibo(i);          // calculte the ith child in the forked process which exits
      else
        c_pids[i] = pid;        // save the pid of this child as a value with key i in the master process
    }

  if(pid != 0){                 // only for the parent process assertion
    for( i = 0; i < nfib; i++){
      waitpid(c_pids[i],&status,0);      // collect the status from the child process with pid mapped from i
      fib[i] = status >> 8;              // right shift by 8 bits to get 8 MSB bits
    }
    for( i = 0; i < nfib; i++)
      fprintf(stdout,"%d ",fib[i]);      // print the array
    fprintf(stdout,"\n");
    fflush(stdout);
  }
  return 0;
}
