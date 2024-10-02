#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<string.h>

char * commands[1000][1000], * input_filename, * output_filename;
int no_of_pipes = 0, no_of_commands = 0;

void parseInputString(char input_str[1000]) {
	char * cmd[1000], * ch, * parsed_input;
	no_of_pipes = 0;
	no_of_commands = 0;	
	// Remove trailing newline character from input string
        input_str[strcspn(input_str, "\n")] = 0;
        parsed_input = input_str;

	// Check for output redirection
	if (strchr(input_str,'>') != NULL) {
		parsed_input = strtok(input_str, ">");
		char * filename = strtok(NULL,">");
		output_filename = strtok(filename, " ");
	}
		
	// Split input string into individual commands using | delimiter
        ch = strtok(parsed_input, "|");
       	while (ch != NULL) {
		cmd[no_of_commands++] = ch;
        	ch = strtok(NULL, "|");
	}
	no_of_pipes = no_of_commands - 1;

	// Check for input redirection
	if (strchr(cmd[0],'<') != NULL) {
                char * c = strtok(cmd[0], "<");
                char * filename = strtok(NULL,"<");
                input_filename = strtok(filename, " ");
        }
	
	// Parse individual commands
	for (int i = 0; i < no_of_commands; i++){
		char * x;
		int n1 = 0;
		x = strtok(cmd[i], " ");
		while (x != NULL) {
			commands[i][n1++] = x;
			x = strtok(NULL, " ");
		}
		commands[i][n1] = NULL;
	}
	// Remove quotes if any
        for (int i = 0; i < no_of_commands; i++) {
                for (int j = 0; commands[i][j] != NULL; j++) {
                        char * pointer;
                        while (pointer = strchr(commands[i][j], '\'')) {
                                memmove(pointer, pointer + 1, strlen(pointer));
                        }
                        while (pointer = strchr(commands[i][j], '\"')) {
                                memmove(pointer, pointer + 1, strlen(pointer));
                        }
                }
        }
}

int main () {

	while(1) {	
		int file_descriptor[100] = {}, status;
		//no_of_commands = 0;
		pid_t pid;
		char input_str[1000];
		input_filename = NULL;
		output_filename = NULL;
		no_of_commands = 0;
		no_of_pipes = 0;
	 	memset(input_str, 0, 1000);
		memset(commands, 0, 1000 * 1000);
		do {
			printf("MyShPrompt> ");
			// Scan input string from command prompt
			fgets(input_str, 1000, stdin);
		}while (strcmp(input_str, "\n") ==0);
		
		// Break loop on entering exit
                if (strcasecmp(input_str,"exit\n") == 0) {
                        break;
                }

		// Parse input string
                parseInputString(input_str);
		
		no_of_commands = no_of_pipes + 1;
		
		// Create pipes
		for (int i = 0; i < no_of_pipes; i++) {
			if (pipe(file_descriptor + i*2) < 0) {
				perror("Error occured while creating the pipe");
				exit(1);	
			}
		}
		for (int i = 0; i < no_of_commands; i++) {
			pid = fork();
			if(pid == 0) {
				// Inside Child process
				
				// Check if first command
				if (i == 0 ){ 
					// Redirect stdout to pipe
					if (dup2(file_descriptor[1], STDOUT_FILENO) < 0 ) {
						perror("Error occured");
						exit(1);
					}
					// Input redirection
					if (input_filename != NULL) {
						// Open input file in read-only mode
		                                int input_fd = open(input_filename, O_RDONLY);
		                                if (input_fd < 0) {
		                                        perror("Error occured while opening the input file");
		                                        exit(1);
		                                }
		                                close(STDIN_FILENO);
						// Redirect stdin to input file descriptor
		                                if (dup2(input_fd, STDIN_FILENO) < 0) {
		                                        perror("Error occured");
		                                        exit(1);
		                                }
		                                close(input_fd);
		                        }
					// Output redirection in the case where # of pipes is zero
					if (no_of_commands == 1) {
						if (output_filename != NULL) {
							// Create output file
		                        	        int output_fd = creat(output_filename, 0640);
		                                	if (output_fd < 0) {
		                                        	perror("Error occured");
		                                       		exit(1);
		                                	}
		                                	close(STDOUT_FILENO);
							// Redirect stdout to output file descriptor
		                                	if (dup2(output_fd, STDOUT_FILENO) < 0) {
		                                        	perror("Error occured while redirecting output");
		                                        	exit(1);
		                               		}
		                                	close(output_fd);
		                        	}
					}

				}
				// Check if last command
				else if (i == (no_of_commands -1)) {
					// Redirect stdin to pipe
					if (dup2(file_descriptor[2*i-2], STDIN_FILENO) < 0) {
						perror("Error occured");
						exit(1);
					}
					// Output redirection
					if (output_filename != NULL) {
						// Create output file
						int output_fd = creat(output_filename, 0640);
						if (output_fd < 0) {
							perror("Error occured");
							exit(1);
						}
						close(STDOUT_FILENO);
						// Redirect stdout to output file descriptor
						if (dup2(output_fd, STDOUT_FILENO) < 0) {
			                                perror("Error occured while redirecting output");
			                                exit(1);
		                                }
						close(output_fd);
					}
				}
				else {
					// Redirect stdin to pipe
					if(dup2(file_descriptor[(2*i)-2], STDIN_FILENO) < 0) {
						perror("Error ocucured");
						exit(1);
					}
					// Redirect stdout to pipe
					if(dup2(file_descriptor[(2*i)+1], STDOUT_FILENO) < 0) {
						perror("Error occurred");
						exit(1);
					}
				}

				// Close read and write descriptors of all pipes
				for (int x = 0; x < (2*no_of_pipes); x++) {
					close(file_descriptor[x]);
				}

				// Execute command
				if (execvp(commands[i][0], commands[i]) < 0) {
					perror("Error occured while executing command");
					exit(1);
				}
			}
		}

		// Close read and write descriptors of all pipes created
		for (int i = 0; i < (2*no_of_pipes); i++) {
			close(file_descriptor[i]);
		}

		// Wait for child process to terminate
		for (int i = 0; i < no_of_commands; i++) {
			wait(&status);
		}
	}
	return 0;
}
