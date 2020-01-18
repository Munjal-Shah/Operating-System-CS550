#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char *args[100];
char commandline[256];
int processid[20] = {0};
char process[100][20];
int counter = 0;

int exeCommand();
int run(char *command);
char * skipSpace(char* x);

pid_t fgpsid = 0;

/**
 * This method handle signal for main process
 * @param sig_num
 * @return
 */
void inthandler(int sig_num) {
	if(fgpsid!=0)
	kill(fgpsid, SIGKILL);
	return;
}


int main() {
	while(1) {
		printf("sh550 >");
		
		//fflush() discards any buffered data that has been fetched from the underlying file, but has not been consumed by the application.
		fflush(stdin);
		char *command;
		
		command = fgets(commandline, 1024, stdin);
		int a = run(command);
	}
	return 0;
}

/**
 * This method shows the list of all
 * processes running in background from array
 */
void showbg() {	
	system("ps");
}

/**
 * This method updates the background process 
 * list if any background process bring to 
 * foreground by fg command
 * @param x
 */
void updatebg(pid_t x) {
	for(int k=0; processid[k]!=0; k++) {
		if(processid[k] == x) {
			processid[k] = 1;
			break;
		}	
	}
}

/**
 * This method kill the background processes 
 * and orphan processes when exit calls
 */
void killbg() {
	int k=0, l=0, status, stat;
	for(k=0; processid[k]!=0; k++) {
		stat = waitpid(processid[k], &status, WNOHANG);
		if(stat==0 && processid[k]!=0) {
			kill(processid[k], SIGKILL);
		}
	}
}

/**
 * This method executes the commands with execvp system calls
 * @return
 */
int exeCommand() {
	
	pid_t pid='\0', result;
	int flag=0, flag2=0, flag3=0, flag4=0;	
	int i = 0;
	char p[100];
	
	while(args[i] != '\0') {
		strcpy(p,args[i]);
		
		//if exit call occur it will call killbg() before exit
		if(strcmp(p, "exit") == 0) {
			killbg();
			exit(0);
		}
		//if & occurs as a last command, flag is set and & is replaced with null
		if(strcmp(p, "&") == 0) {
			args[i] = '\0';
			flag = 1;
			break;		
		}
		//listjobs will call showbg()
		if(strcmp(p, "listjobs") == 0) {
			showbg();
			flag2 = 1;
			break;
		}
		// takes bg process to foreground
		if(strcmp(p, "fg") == 0) {
			flag3 = 1;
			break;
		}
		// flag for implementation of pwd(print working directory)
		if(strcmp(p, "pwd") == 0) {
			flag4 = 1;
			break;
		}
		i++;
	}
	
	// implementation for cd command
	if((args[0]!=NULL) && strcmp(args[0],"cd")==0) {
		
		if(chdir(args[1]) == 0) {
			printf("directory Changed\n");
			return 0;
		}
		else {
			printf("No such directory found\n");
			return 0;	
		}
	}
	
	// implementation of pwd(printing working directory) command
	if(flag4 == 1) {
		char cwd[200];
		if (getcwd(cwd, sizeof(cwd)) == NULL) {
			perror("getcwd() error");
    	}
		else {
			printf("current working directory is: %s\n", cwd);
		}
		return 0;
	}
	
	//if any bg process bring to fg, parent will wait for that to finish
	if(flag3==1 && args[1]!=NULL) {
		
		int returnStatus;
		
		// set foreground process id to current process,for signal handling
		fgpsid = atoi(args[1]);
		
		// update the bg process list
		updatebg(atoi(args[1]));
		
		//wait for process
		result = waitpid(atoi(args[1]), &returnStatus, 0);
		
		return 0;
	}
	
	if(flag2==1){
		printf("\n");
		return 0;
	}
		
	//parent fork a new process
	pid = fork();
	
	if (pid == -1) {
		fprintf(stderr, "fork failed\n"); 
		exit(1); 
	}
 
	// if no & sign in the command than fgpsid is a new created process id
	if(pid>0 && flag==0) {
		fgpsid = pid;
	}	

	if(pid == 0) {
		setpgid(pid,0);

		if (execvp( args[0], args) == -1) {
			perror(args[0]);
			exit(EXIT_FAILURE); 
		}	
	}
	
	// if flag==0 then parent must wait for child
	if(flag != 1) {
		int returnStatus;
			
		// wait for child to complete
		result = waitpid(pid, &returnStatus, 0);
		fflush(stdin);
	}

	// Add new background process id and command for that to the array list and increment the counter
	if(flag==1 && pid>0) {
		char p[100], *s;
		processid[counter] = pid;
		strcpy(p, args[0]);
		strcpy(process[counter], p);
		counter++;
		return 0;
	}
	
	return 0;
}


/**
 * This method separates the arguments and 
 * store it in individual string
 * @param command
 * @return
 */
int run(char *command) {

	char* nextspace;
	char* newline;

	int m = 0;
	
	// skip white space from starting
	command = skipSpace(command);
	
	// last character of command is \n which needs to be replaced with null
	newline = strchr(command,'\n');

	if(newline != '\0') {
		newline[0]='\0';
	}
	
	// breaks whole argument by space between them
	nextspace = strchr(command, ' ');

	if(nextspace != '\0') {
		while(nextspace != '\0') {
			nextspace[0] = '\0';			
			args[m] = command;
			m++;
			command = skipSpace(nextspace + 1);	
			nextspace = strchr(command, ' ');			
		}
	}
	
	if(command[0] != '\0') {
		args[m] = command;
		m++;
	}	
	args[m] = '\0';
	return exeCommand();		
}

/**
 * This method removes space from the command
 * @param x
 * @return
 */
char * skipSpace(char* x) {
	while(isspace(*x)) {
		++x;
	}
	return x;
}