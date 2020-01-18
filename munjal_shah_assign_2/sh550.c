#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h> 


#define MAX_TOKENS 100
# define MAX_STRING_LEN 100
#define EXIT_STR "exit"
# define EXIT_CMD 0
# define UNKNOWN_CMD 99

size_t MAX_LINE_LEN = 10000;
int check_command(int n);
void execute_command();
void stdinProcess(), stdoutProcess();
int process_status = 0;

FILE *fp;
char **tokens;
char *line;
char *stat;
int array = 0;
int store[15];
int ret, status, pointer;
int inputMode();
void sigintHandler();


/**
 * This method initialize file pointer and size to
 * line & tokens array
 */
void initialize() {
	assert((line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);
	assert((tokens = malloc(sizeof(char * ) * MAX_TOKENS)) != NULL);
	assert((fp = fdopen(STDIN_FILENO, "r")) != NULL);
}


/**
 * This method is used for Ctrl+C foreground process termination
 * https://www.geeksforgeeks.org/write-a-c-program-that-doesnt-terminate-when-ctrlc-is-pressed/
 * @param sig_num
 */
void sigintHandler(int sig_num) {
    signal(SIGINT, sigintHandler); 
    printf("\n Foreground process killed by user using ctrl+C \n");
}

/**
 * This method generates tokens and keeps token count
 * @param string
 */
void tokenize(char *string) {
	int token_count = 0;
	int size = MAX_TOKENS;
	char *this_token;
	char cmdline;
	char argv;

	while ((this_token = strsep(&string, " \t\v\f\n\r")) != NULL) {

		if ( *this_token == '\0') continue;
		tokens[token_count] = this_token;
		token_count++;

		if (token_count >= size) {
			size *= 2;
			assert((tokens = realloc(tokens, sizeof(char * ) * size)) != NULL);
		}
	}
	check_command(token_count);
}


/**
 * This method checks which command is encountered and proced accordingly
 * @param n
 * @return
 */
int check_command(int n) {
	
	if (strcmp(tokens[0], EXIT_STR) == 0) {
		printf("Shell Terminated by User \n");
		for (int i = 0; i < 15; i++) {
			if (!store[i] == 0) {
				kill(store[i], SIGTERM);
			}
		}
	}
	
	else if (strcmp(tokens[0], "listjobs") == 0) {
		for (int i = 0; i < 15; i++) {
			if (!store[i] == 0) {
				waitpid(store[i], &status, WNOHANG);
				if ((kill(store[i], 0)) == 0) {
					stat = "Running";
				} else {
					stat = "Finished";
				}
				printf("Command with pid %d | Status:%s \n", store[i], stat);
			}
		}
	}
	
	else if (strcmp(tokens[0], "fg") == 0) {
		int pidid = atoi(tokens[1]);
		int waitpidret = waitpid(pidid, NULL, 0);
		kill(waitpidret, SIGCONT);
	}
	
	else if (strcmp(tokens[n-1], "&") == 0) {
		process_status = 1;
		tokens[n-1] = NULL;
		execute_command();
	}
	
	else {
		int i;
		
		for (i=0; i<n; ++i) {
			if (strcmp(tokens[i], ">") == 0) {
				tokens[i] = NULL;
				pointer = i;
				stdoutProcess();
				break;
			}
			else if (strcmp(tokens[i], "<") == 0) {
				tokens[1] = tokens[2];
				tokens[2] = NULL;
				stdinProcess();
				break;
			}
		}
		
		if (i == n) {
			process_status = 2;
			execute_command();
		}
	}
}


/**
 * This method reads command from line
 */
void read_command() {
	assert(getline(&line, &MAX_LINE_LEN, fp) > -1);
	tokenize(line);
}


/**
 * This method calls fork method and creates a child
 */
void execute_command() {
	pid_t pid;
	
	if ((pid=fork()) < 0) {
		printf("Forking the Child Process Failed \n");
		exit(1);
	}
	
	else if (pid == 0) {
		if (execvp(tokens[0], tokens) < 0) {
			printf("Command Execution Failed! \n");
			exit(1);
		}
	}
	
	else {
		if (process_status == 1) {
			while (waitpid(pid, & status, WNOHANG));
			store[array] = pid;
			array++;
			printf("PID of Current Process is %d \n", store[array-1]);
		}
		
		else if (process_status == 2) {
			waitpid(pid, & status, 0);
			store[array] = pid;
			array++;
			printf("PID of Current Process is %d \n", store[array-1]);
		}
	}
}


/**
 * This method is called when exit is encountered
 */
int run_command() {
	if (strcmp(tokens[0], EXIT_STR) == 0) {
		return EXIT_CMD;
	}
	return UNKNOWN_CMD;
}


/**
 * This method is called when input from a file is given as a parameter to command
 */
void stdinProcess() {
	pid_t pid;
	
	if ((pid=fork()) < 0) {
		printf("Forking the Child Process Failed \n");
	}
	
	int fd;
	
	if (pid == 0) {
		fd = open(tokens[1], O_RDONLY);
		if (fd < 0) {
			printf("Open Failed \n");
		}
		int d = dup2(fd, 0);
		if (d == 0) {
			close(fd);
			
			if ((execvp(tokens[0], tokens)) < 0) {
				printf("Exec Failed: Invalid command \n");
			}
		} else {
			perror("Dup Error:");
			close(fd);
			exit(0);
		}
	}
	
	if (pid > 0) {
		waitpid(pid, & status, 0);
	}
}


/**
 * This method is called when output of the command is written to file
 */
void stdoutProcess() {
	pid_t pid;
	
	if ((pid=fork()) < 0) {
		printf("Forking the Child Process Failed \n");
	}
	
	int fd;
	
	if (pid == 0) {
		fd = creat(tokens[pointer + 1], 0644);
		if (fd < 0) {
			printf("Fd Failed to create a file \n");
		}
		int d = dup2(fd, 1);
		if (d < 0) {
			perror("Dup Error: ");
			close(fd);
			exit(0);
		} else {
			close(fd);
		
			if ((execvp(tokens[0], tokens)) < 0) {
				printf("Exec Failed: Invalid command \n");
			}
		}
	}
	if (pid > 0) {
		waitpid(pid, &status, 0);
	}
}


int main() {
	initialize();
	signal(SIGINT, sigintHandler);
	
	do {
		printf("sh550> ");
		read_command();
		printf("\n");
	} while (run_command() != EXIT_CMD);
	
	return 0;
}