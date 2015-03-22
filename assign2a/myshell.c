#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>

#define MAX_INPUT_SIZE 100
#define MAX_TOKENS 10
#define SPACES " \r\t\n"
#define BYTE_COUNT 256

int printlogo();
int create_shell();
int parse(char*);
int tokenize(char*, char* []);
int pwd();
int cd(char*);
int mkdir_(char*);
int rmdir_(char*);
int ls(int);
int cp(char*, char*);
int execute(char* [],int);

// global exit flag
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
  printlogo();
  char inpt[MAX_INPUT_SIZE];
  char prompt[MAX_INPUT_SIZE];
  //unless exit flag is set
  while(exit_ != 1){
    if(getcwd(prompt,sizeof(prompt)) != NULL){
      fprintf(stdout,"%s",prompt);
      fprintf(stdout,"%s",">");
      fgets(inpt,sizeof(inpt),stdin);
      parse(inpt);
    }
    else{
      // prompterror: if present working directory is altered
      perror("Prompt");
    }
  }
  return 0;
}

//parse the input
int parse(char *input){

  char* tkns[MAX_TOKENS];
  char *pos;
  int i,tkn_count;
  //remove the trailing newline from the input
  if ((pos=strchr(input, '\n')) != NULL)
    *pos = '\0';

  //allocate space for tokens
  for(i = 0;i < MAX_TOKENS;i++)
    tkns[i] = (char*)malloc(sizeof(char));

  char cmmnd[MAX_INPUT_SIZE];
  //tokenize
  tkn_count = tokenize(input,tkns);

  //command = first token
  strcpy(cmmnd,tkns[0]);
  //execute function assosciated with token
  //null statement reproduces prompt
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
    //checking fot the '-l' flag
    if(strcmp(tkns[1],"-l") == 0)
      ls(1);
    else
      ls(0);
  }
  else if(strcmp(cmmnd,"cp") == 0)
    cp(tkns[1],tkns[2]);
  else if(strcmp(cmmnd,"exit") == 0)
    exit_ = 1;
  else
    //execute if not a built in command
    execute(tkns,tkn_count);
    // free the tokens
    for(i = 0;i < MAX_TOKENS;i++)
      free(tkns[i]);
  return 0;
}

// tokenize user input with space as delimiters
int tokenize(char *input,char *tokens[]){
  char str[MAX_INPUT_SIZE];
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
    //filename does not exists or insuffecient permissions
    perror("cd");
  return 0;
}

// present working directory
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

// make a new directory
int mkdir_(char *path_dir){
  // with default read write and search permissions for the owner
  if(mkdir(path_dir,0700) != 0)
    perror("mkdir");
  return 0;
}

int rmdir_(char *path_dir){
  if(rmdir(path_dir) != 0)
    perror("rmdir");
  return 0;
}

int ls(int l){

  DIR *dp;
  struct dirent *ep;
  struct stat fileStat;
  dp = opendir ("./");
  if (dp != NULL){
      while (ep = readdir (dp),ep != 0){
          if(l == 1){
            // long list format
            if(stat(ep->d_name,&fileStat) == 0){
              char time[14];
              // get the Month Day Hour Minutes Format
              strftime (time,14,"%h %d %H:%M",localtime(&fileStat.st_mtime));
              // get the filetype(directory or not)
              fprintf(stdout,"%1s",(S_ISDIR(fileStat.st_mode)) ? "d" : "-");
              // owner permissions
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IRUSR) ? "r" : "-");
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IWUSR) ? "w" : "-");
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IXUSR) ? "x" : "-");
              // group permissions
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IRGRP) ? "r" : "-");
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IWGRP) ? "w" : "-");
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IXGRP) ? "x" : "-");
              // others permissions
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IROTH) ? "r" : "-");
              fprintf(stdout,"%1s",(fileStat.st_mode & S_IWOTH) ? "w" : "-");
              fprintf(stdout,"%1s ",(fileStat.st_mode & S_IXOTH) ? "x" : "-");
              // number of hard links assisciated
              fprintf(stdout,"%4ld ",(unsigned long)(fileStat.st_nlink));
              // owner name and owners group name
              fprintf(stdout,"%s ",(getpwuid(fileStat.st_uid))->pw_name);
              fprintf(stdout,"%s ",(getgrgid(fileStat.st_gid))->gr_name);
              // file size in bytes
              fprintf(stdout,"%5lld ",(unsigned long long)fileStat.st_size);
              // time
              fprintf(stdout,"%s ",time);
            }
            else
              // error if staistics are not accessible
              perror("ls");
          }
          fprintf (stdout,"%s\n",ep->d_name);
        }
        // close the directory heirarchy stream
      (void) closedir (dp);
    }
  else
    // if directory permissions are not available
    perror ("ls");
  return 0;
}


int cp(char *path_file1,char *path_file2){
  FILE* from;
  FILE* to;
  char buffer[BYTE_COUNT];
  struct stat from_stat;
  struct stat to_stat;
  time_t from_mtime;
  time_t to_mtime;
  size_t bytes_read;

  // opena as a readable binary
  from  = fopen(path_file1,"rb");
  // if source file cannot be opened
  if(from == NULL){
    perror("cp:source file");
    return 0;
  }

  // to get the modified statistics of the source file
  stat(path_file1,&from_stat);
  from_mtime = from_stat.st_mtime;

  // reset error codes
  errno = 0;
  // create a non existent file to write in binary
  to = fopen(path_file2,"wbx");
  // destination stream not opened
  if(to == NULL){
    //  if the file to be created for writing already exists
    if(errno == EEXIST){
      // get its modified time
      stat(path_file2,&to_stat);
      to_mtime = to_stat.st_mtime;
      // if existing file is older than source file overwrite
      if(to_mtime < from_mtime)
        to = fopen(path_file2,"wb");
      else{
        // replacing newer file error
        fprintf(stderr,"cp:destination file modified later than the source file\n");
        fclose(from);
        return 0;
      }
    }
    // if destination stream still not opened due to permissions,etc
    if(to == NULL){
      perror("cd:destination file");
      fclose(from);
      return 0;
    }
  }

  // the copy process
  while (bytes_read != EOF) {
     bytes_read = fread(buffer, 1, BYTE_COUNT, from);
     // nothing read
     if (bytes_read == 0)
       break;
     fwrite(buffer, 1, bytes_read, to);
  }
  //no status generated if copy is successful
  // close the streams
  fclose(from);
  fclose(to);
  return 0;
}

int execute(char *tkns[],int count){
  int i;
  // copy the tokens into a execvp permissible format -- double pointer
  char **tokens = (char**)malloc(sizeof(char*));
  for(i = 0;i< count ; i++ )
    tokens[i] = tkns[i];
  pid_t pid;
  // new process
  pid = fork();
  if(pid == 0){
    // replace the child with the process
    execvp(tokens[0],tokens);
    // if execvp exits then error
    perror(tkns[0]);
    // exit the child
    _exit(0);
  }
  else{
    // if last token is & then do not wait
    if(strcmp(tokens[count-1],"&") != 0)
      // wait for the child process to finish
      waitpid(pid,NULL,0);
    else
      // print the pid for the new background process
      fprintf(stdout,"PID:%d\n",pid);
  }
  return 0;
}

int printlogo(){
  fprintf(stdout,"..7 ..                              ....\n.     .                          .......\n.     ..                     .....: 7...\n.      .                  .......     ..\n..     ..               ........      ..\n .      .             ..........      ..\n ..     ..          ...........      ...\n  .      .        .............      ...\n  ..     ..   ................:      ...\n   .      .  ..    ...........      ....\n   .:     .  .     =.     ....      ....\n    .     ....     ..     ...      .....\n    .      ...     ..     ...      .....\n    ..     ...     .      ...      .....\n     .      ..     .      ..      ......\n     ..     ..     .      ..      ......\n     .?.:         ..     ~.I      ......\n     ...            .    ..      +..:=I \n     .=             .    ..      ..7    \n    .7             ..    .7      .      \n   .      ......~ ..    ..       .      \n  .             ... ....:       =.      \n  ..              ..            ..      \n   .               ..           ..      \n   ..               ..          .       \n    .               ?.         ..       \n   ...               .         .        \n   ...                        ..        \n   ....                       .         \n  .....                      ..         \n  ......                     .          \n  ............................\n ");
  fprintf(stdout,"Welcome to MyShell 1.0\n");
  fprintf(stdout,"- Nevin Valsaraj 12CS10032\n");
  fprintf(stdout,"- Pranjal Pandey 12CS30026\n");
  return 0;
}
