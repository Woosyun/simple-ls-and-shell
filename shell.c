#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXARGS 128
#define MAXCOMMAND 12
#define READ 0
#define WRITE 1

char *getinput(char *buffer, size_t buflen) {
	printf("$ ");
	return fgets(buffer, buflen, stdin);
}

void makeCmd(char *buf, char *cmd[MAXARGS]){
	int idx=0;
	cmd[idx] = strtok(buf, " ");
	while(cmd[idx] != NULL){
		idx++;
		cmd[idx] = strtok(NULL, " ");
	}
}

int makeArgs(char **cmd, char *args[MAXARGS][MAXCOMMAND]){
	int row=0;
	int col=0;
	int idx=0;
	while(cmd[idx]!=NULL){
		if(!strcmp(cmd[idx], "|")){
			row++;
			col=0;
		} else {
			args[row][col] = cmd[idx];
			col++;
		}
		idx++;
	}

	return row+1;
}

int findSig(char **cmd, char *sig){
	int idx=0;
	while(cmd[idx] != NULL){
		if(!strcmp(cmd[idx], sig))
			return idx;
		idx++;
	}
	return -1;
}

void pipeLine(int argn, char *args[MAXARGS][MAXCOMMAND]){
	//make pipes
	int fd[MAXARGS-1][2];
	for(int i=0; i<argn-1; i++){
		if(pipe(fd[i])<0){
			perror("pipe()");
			exit(1);
		}
	}

	//execute commands
	for(int i=0; i<argn; i++){
		pid_t pid = fork();
		
		if(pid < 0){
			perror("fork()");
			exit(1);
		} else if (pid == 0){
			//connect previous process's read fd to standard input fd
			if(i>0){
				if(dup2(fd[i-1][READ], STDIN_FILENO) < 0){
					perror("dup2()");
					exit(1);
				}
			}
			//connect current process's write fd to standard output fd
			if(i < argn -1) {
				if(dup2(fd[i][WRITE], STDOUT_FILENO) < 0) {
					perror("dup2()");
					exit(1);
				}
			}

			for(int j=0; j<argn-1; j++){
				close(fd[j][0]);
				close(fd[j][1]);
			}

			//execute and pass to pipe
			if(execvp(args[i][0], args[i])<0){
				perror("execvp()");
				exit(1);
			}
		}
	}

	for(int i=0; i<argn-1; i++){
		close(fd[i][0]);
		close(fd[i][1]);
	}

	for(int i=0; i<argn; i++){
		wait(NULL);
	}
}

void runCommandLine(char *cmdLine){
	char buf[BUFSIZ];
    strcpy(buf, cmdLine);

	//change buffer to comamnd list
	char *cmd[MAXARGS];
	makeCmd(buf, cmd);

	int idx;
	int fd;

	//I/O redirection
	idx = findSig(cmd, ">");
	if(idx != -1) {
		cmd[idx] = NULL;
		char outputFileName[255];
		strcpy(outputFileName, cmd[idx+1]);
		fd = open(outputFileName, O_RDWR|O_CREAT|O_TRUNC, 0644);
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}
	idx = findSig(cmd, ">>");
	if(idx != -1) {
		cmd[idx] = NULL;
		char outputFileName[255];
		strcpy(outputFileName, cmd[idx+1]);
		fd = open(outputFileName, O_RDWR|O_CREAT|O_APPEND, 0644);
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}
	idx = findSig(cmd, "<");
	if(idx != -1){
		cmd[idx] = cmd[idx+1];
		cmd[idx+1] = NULL;
	}

	//make 1 dimenssion command line to 2 dimenssion for execute
	char *args[MAXARGS][MAXCOMMAND];
	int argn = makeArgs(cmd, args);

	//run commands in pipe line
	pipeLine(argn, args);
}

void sigint_handler(int sig){
	printf("You are in my custom shell. See you again. Bye!\n");
	exit(0);
}

int main(int argn, char **args) {
	char buf[BUFSIZ];
	pid_t pid;
	int status;

	/* cast to void to silence compiler warnings */
	(void)argn;
	(void)args;

	/* 2.1 trap the SIGINT*/
	if (signal(SIGINT, sigint_handler) == SIG_ERR)
		fprintf(stderr, "%s: %s\n", "signal error", strerror(errno));
	
	/* run the shell */
	while (getinput(buf, sizeof(buf))) {
		buf[strlen(buf) - 1] = '\0';

		if((pid=fork()) == -1) {
			fprintf(stderr, "shell: can't fork: %s\n", strerror(errno));
			continue;
		} else if (pid == 0) {
			/* run the command line input */
			runCommandLine(buf);
			exit(0);
		}

		if ((pid=waitpid(pid, &status, 0)) < 0) {
			fprintf(stderr, "shell: waitpid error: %s\n", strerror(errno));
		}
	}

	exit(EX_OK);
}
