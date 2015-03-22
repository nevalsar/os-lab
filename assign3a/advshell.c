/* 
	PRANJAL PANDEY 12CS30026
	NEVIN VALSARAJ 12CS10032
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>

#define MAX_INPUT_SIZE 100
#define MAX_TOKENS 10
#define SPACES " \r\t\n"
#define PIPE "|"
#define FILE_IN "<"
#define FILE_OUT ">"
#define BYTE_COUNT 256


// global exit flag
typedef int flag;

int printlogo();
int create_shell();
int parse(char*);
int parse_filedump(char*,int*,int*);
int parse_filesource(char*,int*,int*);
int parse_command(char*,int*,int*);
int tokenize(char*, char* [],char *);
char* trimwhitespace(char*);
int pwd(int*,int*);
int cd(char*,int*,int*);
int mkdir_(char*,int*,int*);
int rmdir_(char*,int*,int*);
int ls(int,int*,int*);
int cp(char*, char*, int*,int*);
int execute(char* [],int,int*,int*);


flag exit_;
flag background_;
int pipelines[MAX_TOKENS][2];
int write_fd[2];
int read_fd[2];
pid_t processes[MAX_TOKENS];
int process_count;

//shell process executes from here
int main(){
	exit_ = 0;
	background_ = 0;
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
			process_count = 0;
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

//parse pipelines
int parse(char *input){
	char *tkns[MAX_TOKENS];
	char *pos;
	int i,tkn_count;
  	//remove the trailing newline from the input
	if ((pos=strchr(input, '\n')) != NULL)
		*pos = '\0';

		// fprintf(stderr, "one commands\n" );
  	//allocate space for tokens
	for(i = 0;i < MAX_TOKENS;i++)
		tkns[i] = (char*)malloc(sizeof(char));

	tkn_count = tokenize(input,tkns,PIPE);

	if(tkn_count > 1){
		for(i = 0;i < tkn_count-1; i++){
			if(pipe(pipelines[i]) == -1)
				perror("pipe");
		}

		parse_filedump(tkns[0],NULL,pipelines[0]);
		for(i = 1 ; i < tkn_count -1; i++){
			parse_filedump(tkns[i],pipelines[i-1],pipelines[i]);
		}
		parse_filedump(tkns[i],pipelines[i-1],NULL);
		
		// for(i = 0;i < tkn_count-1; i++){
		// 	close(pipelines[i][0]);
		// 	close(pipelines[i][1]);
		// }
		if(!background_)
			waitpid(processes[process_count-1],NULL,0);
	}
	else{
		parse_filedump(input,NULL,NULL);
		if(!background_)
			wait(NULL);
	}
	
	return 0;
}

//parse redirect operators
int parse_filedump(char *input,int *read,int *write){
	char *tkns[MAX_TOKENS];
	int i,tkn_count;

  	//allocate space for tokens
	for(i = 0;i < MAX_TOKENS;i++)
		tkns[i] = (char*)malloc(sizeof(char));

	tkn_count = tokenize(input,tkns,FILE_OUT);

	if(tkn_count == 2){
		if(write != NULL){
			fprintf(stderr, "Pipe out and File Out Redirect Not Supported Simultaneously");
			return 0;
		}
		write_fd[0]=12;
		if((write_fd[1]=open(tkns[1],O_CREAT | O_WRONLY | O_TRUNC,S_IRWXU | S_IRWXG | S_IRWXO)) < 0){
			perror("Output File");
			return 0;
		}
		
		parse_filesource(tkns[0],read,write_fd);
		close(write_fd[1]);
	}
	else if(tkn_count == 1){
		parse_filesource(input,read,write);
	}
	else{
		fprintf(stderr, "More than 1 out-file redirect not supported");
		return 0;
	}
	
	return 0;
}

int parse_filesource(char *input,int *read,int *write){
	
	char *tkns[MAX_TOKENS];
	int i,tkn_count;

  	//allocate space for tokens
	for(i = 0;i < MAX_TOKENS;i++)
		tkns[i] = (char*)malloc(sizeof(char));

	tkn_count = tokenize(input,tkns,FILE_IN);

	if(tkn_count == 2){
		if(read != NULL){
			fprintf(stderr, "Pipe in and File-In Redirect Not Supported Simultaneously");
			return 0;
		}
		read_fd[1]=11;
		if((read_fd[0]=open(tkns[1], O_RDONLY)) < 0){
			perror("Input File");
			return 0;
		}
		
		parse_command(tkns[0],read_fd,write);
		close(read_fd[0]);
	}
	else if(tkn_count == 1){
		parse_command(input,read,write);
	}
	else{
		fprintf(stderr, "More than 1 in-file redirect not supported");
		return 0;
	}
	return 0;	
}

//parse the atomic commands
int parse_command(char *input,int* pipe1,int* pipe2){

	char* tkns[MAX_TOKENS];
	int i,tkn_count;

  //allocate space for tokens
	for(i = 0;i < MAX_TOKENS;i++)
		tkns[i] = (char*)malloc(sizeof(char));

	char cmmnd[MAX_INPUT_SIZE];
  //tokenize
	tkn_count = tokenize(input,tkns,SPACES);
  //command = first token
	strcpy(cmmnd,tkns[0]);
  //execute function assosciated with token
  //null statement reproduces prompt
	if(cmmnd == NULL);
	else if(strcmp(cmmnd,"cd") == 0)
		cd(tkns[1],pipe1,pipe2);
	else if(strcmp(cmmnd,"pwd") == 0)
		pwd(pipe1,pipe2);
	else if(strcmp(cmmnd,"mkdir") == 0)
		mkdir_(tkns[1],pipe1,pipe2);
	else if(strcmp(cmmnd,"rmdir") == 0)
		rmdir_(tkns[1],pipe1,pipe2);
	else if(strcmp(cmmnd,"ls") == 0){
    //checking fot the '-l' flag
		if(strcmp(tkns[1],"-l") == 0)
			ls(1,pipe1,pipe2);
		else
			ls(0,pipe1,pipe2);
	}
	else if(strcmp(cmmnd,"cp") == 0)
		cp(tkns[1],tkns[2],pipe1,pipe2);
	else if(strcmp(cmmnd,"exit") == 0)
		exit_ = 1;
	else
    //execute if not a built in command
		execute(tkns,tkn_count,pipe1,pipe2);
    // free the tokens
	if(pipe1 != NULL){
		close(pipe1[0]);
		close(pipe1[1]);
	}

	for(i = 0;i < MAX_TOKENS;i++)
		free(tkns[i]);
	return 0;
}

// tokenize user input with space as delimiters
int tokenize(char *input,char *tokens[], char *DELIM){
	char str[MAX_INPUT_SIZE];
	char* tkn;
	unsigned short int i,count;
	i = count = 0;
	strcpy(str,input);
	tkn = strtok (str,DELIM);
	while (tkn != NULL)
	{
		tkn = trimwhitespace(tkn); 
		strcpy(tokens[i++],tkn);
		count++;
		tkn = strtok (NULL,DELIM);
	}
	return count;
}

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}


int cd(char *path_dir,int* in,int* out){
	if(chdir(path_dir) != 0)
    //filename does not exists or insuffecient permissions
		perror("cd");
	return 0;
}

// present working directory
int pwd(int* in,int* out){
	char pwd[100];
	FILE* fp_out = stdout;		
	if(out != NULL)
		fp_out = fdopen(out[1],"wb");
	if(getcwd(pwd,sizeof(pwd)) != NULL){
		fprintf(fp_out,"%s\n",pwd);
	}
	else{
		perror("pwd");
	}
	if(out!=NULL)
		fclose(fp_out);
	return 0;
}

// make a new directory
int mkdir_(char *path_dir,int* in,int* out){
  // with default read write and search permissions for the owner
	if(mkdir(path_dir,0700) != 0)
		perror("mkdir");
	return 0;
}

int rmdir_(char *path_dir,int* in, int* out){

	if(rmdir(path_dir) != 0)
		perror("rmdir");
	return 0;
}

int ls(int l,int* in,int* out){
	FILE* fp_out = stdout;
	if(out != NULL)
		fp_out = fdopen(out[1],"wb");
	DIR *dp;
	
	struct stat fileStat;
	dp = opendir ("./");
	if (dp != NULL){
		struct dirent *ep;
		while (ep = readdir (dp),ep != 0){
			if(l == 1){
            // long list format
				if(stat(ep->d_name,&fileStat) == 0){
					char time[14];
              // get the Month Day Hour Minutes Format
					strftime (time,14,"%h %d %H:%M",localtime(&fileStat.st_mtime));
              // get the filetype(directory or not)
					fprintf(fp_out,"%1s",(S_ISDIR(fileStat.st_mode)) ? "d" : "-");
              // owner permissions
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IRUSR) ? "r" : "-");
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IWUSR) ? "w" : "-");
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IXUSR) ? "x" : "-");
              // group permissions
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IRGRP) ? "r" : "-");
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IWGRP) ? "w" : "-");
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IXGRP) ? "x" : "-");
              // others permissions
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IROTH) ? "r" : "-");
					fprintf(fp_out,"%1s",(fileStat.st_mode & S_IWOTH) ? "w" : "-");
					fprintf(fp_out,"%1s ",(fileStat.st_mode & S_IXOTH) ? "x" : "-");
              // number of hard links assisciated
					fprintf(fp_out,"%4ld ",(unsigned long)(fileStat.st_nlink));
              // owner name and owners group name
					fprintf(fp_out,"%s ",(getpwuid(fileStat.st_uid))->pw_name);
					fprintf(fp_out,"%s ",(getgrgid(fileStat.st_gid))->gr_name);
              // file size in bytes
					fprintf(fp_out,"%5lld ",(unsigned long long)fileStat.st_size);
              // time
					fprintf(fp_out,"%s ",time);
				}
				else
              // error if staistics are not accessible
					perror("ls");
			}
			fprintf (fp_out,"%s\n",ep->d_name);
		}
        // close the directory heirarchy stream
		(void) closedir (dp);
	}
	else
    // if directory permissions are not available
		perror ("ls");

	if(out!=NULL)
		fclose(fp_out);
	return 0;
}


int cp(char *path_file1,char *path_file2,int* in,int* out){

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

int execute(char *tkns[],int count,int* in,int* out){
	
	int i;
	//fprintf(stderr, "%d %d\n",in,out);
  //copy the tokens into a execvp permissible format -- double pointer
	char *tokens[count+1];

	background_ = 0;

	for(i = 0;i < count; i++ ){
		tokens[i] = (char*) malloc(sizeof(char));
		sprintf(tokens[i],"%s",tkns[i]);
	}
	tokens[i]=NULL;

	
	pid_t pid;
  // new process
	pid = fork();
	if(pid == 0){
    // replace the child with the process
		if(in != NULL){
			close(in[1]);	
			dup2(in[0],STDIN_FILENO);
			close(in[0]);
		}
		if(out != NULL){
			close(out[0]);
			dup2(out[1],STDOUT_FILENO);
			close(out[1]);
		}
		execvp(tokens[0],tokens);
    // if execvp exits then error
		perror(tokens[0]);
    // exit the child
		_exit(0);
	}
	else if(pid < 0)
		perror("fork");
	else{
		// if(in!=NULL){
		//  	close(in[0]);
		// 	close(in[1]);
		// }
    // if last token is & then do not wait
		if(strcmp(tokens[count-1],"&") != 0)
      // wait for the child processes to finish
			processes[process_count++]=pid;
			//waitpid(pid,NULL,0);
		else{
      // print the pid for the new background process
			fprintf(stdout,"PID:%d\n",pid);
			background_ = 1;
		}
	}
	return 0;
}

int printlogo(){
	fprintf(stdout,"..7 ..                              ....\n.     .                          .......\n.     ..                     .....: 7...\n.      .                  .......     ..\n..     ..               ........      ..\n .      .             ..........      ..\n ..     ..          ...........      ...\n  .      .        .............      ...\n  ..     ..   ................:      ...\n   .      .  ..    ...........      ....\n   .:     .  .     =.     ....      ....\n    .     ....     ..     ...      .....\n    .      ...     ..     ...      .....\n    ..     ...     .      ...      .....\n     .      ..     .      ..      ......\n     ..     ..     .      ..      ......\n     .?.:         ..     ~.I      ......\n     ...            .    ..      +..:=I \n     .=             .    ..      ..7    \n    .7             ..    .7      .      \n   .      ......~ ..    ..       .      \n  .             ... ....:       =.      \n  ..              ..            ..      \n   .               ..           ..      \n   ..               ..          .       \n    .               ?.         ..       \n   ...               .         .        \n   ...                        ..        \n   ....                       .         \n  .....                      ..         \n  ......                     .          \n  ............................\n ");
	fprintf(stdout,"Welcome to MyShell 1.1\n");
	fprintf(stdout,"- Nevin Valsaraj 12CS10032\n");
	fprintf(stdout,"- Pranjal Pandey 12CS30026\n");
	return 0;
}