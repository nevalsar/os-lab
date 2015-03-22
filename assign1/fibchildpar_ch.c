/*
Nevin Valsaraj
12CS10032
Pranjal Pandey
12CS30026
Assignment 1 Part 3
Prints 'multiset' of n fibonacci numbers
*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


// calcultes the ith fibonacci number and prints it
void forkedfibo(int i){
  int a,b,c;
  a = b = c = 1;
  if(i == 0 || i == 1){
    fprintf(stdout,"%d ",1);
  }
  else {
    while(i >= 2){
      c = a + b;
      a = b;
      b = c;
      i--;
    }
    fprintf(stdout,"%d ",c);
  }
  fflush(stdout);
}

int main(){
  int nfib,i;
  pid_t pid,last_pid,master_pid;

  master_pid = pid = getpid();

  scanf("%d",&nfib);

  for(i=0;i<nfib;i++)
    if(getpid() == master_pid){           // only master process loops
      pid = fork();                       // fork a child

      if(pid == 0){
        if(i!=0)
          waitpid(last_pid);              // next child wait for the last child to finish
        forkedfibo(i);                    // forkedfibo which prints
        break;                            // exit child to finish
      }
      else{
        last_pid = pid;                 // update last pid witht this pid in this process
      }
    }

    waitpid(last_pid);                     // parent wait for the last child forked to finish
    if(pid != 0){
      fprintf(stdout,"\n");
      fflush(stdout);
    }

    return 0;
}
