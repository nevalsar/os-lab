#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_INPUT 100
#define MAX_TOKENS 10
#define SPACES " \r\t\n"

int create_shell();
int parse(char*);
int tokenize(char*, char [][MAX_INPUT]);
int pwd();
int cd(char*);
int mkdir_(char*);
int rmdir_(char*);
int ls(int);
int cp(char*, char*);

typedef int flag;
flag exit_;

//shell process executes from here
int main(){
  exit_ = 0;
  create_shell();
  return 0;
}

// create the shell and prompt
int create_shell(){

  char inpt[MAX_INPUT];
  char prompt[MAX_INPUT];

  while(exit_ != 1){
    char* pos;
    if(getcwd(prompt,sizeof(prompt)) != NULL){
      fprintf(stdout,"%s",prompt);
      fprintf(stdout,"%s",">");
      fgets(inpt,sizeof(inpt),stdin);
      if ((pos=strchr(inpt, '\n')) != NULL)
      *pos = '\0';
      parse(inpt);
    }
    else{
      perror("Prompt");
    }
  }
  return 0;
}

//parse the input
int parse(char *input){

  char tkns[MAX_TOKENS][MAX_INPUT];
  char cmmnd[MAX_INPUT];
  tokenize(input,tkns);
  strcpy(cmmnd,tkns[0]);
  if(cmmnd == NULL);
  else if(strcmp(cmmnd,"cd") == 0)
    cd(tkns[1]);
  else if(strcmp(cmmnd,"pwd") == 0)
    pwd();
  else if(strcmp(cmmnd,"mkdir") == 0)
    mkdir_(tkns[1]);
  else if(strcmp(cmmnd,"rmdir") == 0)
    rmdir_(tkns[1]);
  else if(strcmp(cmmnd,"ls") == 0){
    if(strcmp(tkns[1],"-l") == 0)
      ls(1);
    else
      ls(0);
  }
  else if(strcmp(cmmnd,"cp") == 0)
    cp(tkns[1],tkns[2]);
  else if(strcmp(cmmnd,"exit") == 0)
    exit_ = 1;
  return 0;
}

// tokenize user input with space as delimiters
int tokenize(char *input,char tokens[][MAX_INPUT]){
  char str[MAX_INPUT];
  char* tkn;
  unsigned short int i,count;
  i = count = 0;
  strcpy(str,input);
  tkn = strtok (str,SPACES);
  while (tkn != NULL)
  {
    strcpy(tokens[i++],tkn);
    count++;
    tkn = strtok (NULL," ");
  }
  return count;
}

int cd(char *path_dir){
  if(chdir(path_dir) != 0)
    perror("cd");
  return 0;
}

int pwd(){
  char pwd[100];
  if(getcwd(pwd,sizeof(pwd)) != NULL){
    fprintf(stdout,"%s\n",pwd);
  }
  else{
    perror("pwd");
  }
    return 0;
}

int mkdir_(char *path_dir){
  return 0;
}

int rmdir_(char *path_dir){
  return 0;
}

int ls(int l){
  return 0;
}

int cp(char *path_file1,char *path_file2){
  return 0;
}
