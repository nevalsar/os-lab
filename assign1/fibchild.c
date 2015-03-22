/*
Nevin Valsaraj
12CS10032
Pranjal Pandey
12CS30026
Assignment 1 Part 1
Prints ordered set of fibonacci numbers by executing child process sequenced by a waiting master process.
*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(){
  int a,b,n;
  pid_t pid;
  a = 1;
  b = 1;
  n = 10;
  pid = getpid();               // initialised with an unforked parent process
  scanf("%d",&n);
  //BASE CASES
  if( n>=1){
    fprintf(stdout,"%d ",a);
    n--;
  }
  if( n>=1){
    fprintf(stdout,"%d ",b);
    n--;
  }
  fflush(stdout);        // flush so that buffers do not interfere with parents
  //LOOP TILL REQUIRED NUMBERS PRINTED
  while(n>0){
    pid = fork();                         // fork a child process
    if(pid == 0){
      fprintf(stdout,"%d ",a+b);
      fflush(stdout);                     // flush each child after print buffers to prevent multiple outputs
      break;
    }
    else{
      wait();                             // wait for the last child to print ( and flush)
      b += a;
      a = b-a;
      n--;
    }
  }
  if(pid != 0)                            // parent prints newline before exit
    fprintf(stdout,"\n");
    fflush(stdout);

    return 0;                             // yet to discover the exit codes
  }
