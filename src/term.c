#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

//Constants
#define BUFFER_SIZE 		1024
#define MAX_ARGS 			64
#define MAX_FLAGS 			6
#define RUN_FLAG 			0
#define QUIT_FLAG 			1
#define HELP_FLAG 			2
#define MURDER_FLAG 		3
#define BACKGROUND_FLAG 	4
#define SCRIPT_FLAG 		5

#define SCRIPT		"script"
#define RUN 		"run"
#define PROMPT 		"# "
#define HELP 		"help"
#define QUIT 		"quit"
#define MURDER 		"murder"
#define BACKGROUND 	"background"
#define DELIMITERS 	" \n\t"
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define CHILD_PID 0

#define HELP_PROMPT "Commands:\n\trun path\tRuns the program located at path\n\tbackground path\tRuns the program located at path in the background\n\tmurder pid\tKills the process specified by pid\n\thelp\t\tDisplays this menu\n\t\t\n\n\tSee README for more information\n"

//Global flag array
int flags[MAX_FLAGS];

//function prototypes
void run(char** args);
void murder(char** args);
void background(char** args);

void printArray(char** a);
void core();
void prompt();
int script();
void helpPrompt();
void flushFlags();
void inCheck(char** args);
void clearBuffer(char buf[]);
void getArgs(char* in, char** args);
void printStringArray(char** array);

int main(int argc, char** argv) {
	core();
	return 0;
}

/*
	Program driver everything is run from here
*/
void core() {

	/*
		arg:	Working pointer to args
		args:	String array for arguments
		input:	Input string from commandline

	*/
	char** arg;
	char* args[MAX_ARGS];
	char input[BUFFER_SIZE];

	while(true) {
		//Prompt and get input
		prompt();
		fflush(stdout);
		fgets(input, BUFFER_SIZE, stdin);
		
		//Parse input into args array and check for commands
		getArgs(input, args);
		inCheck(args);

		pid_t pID;

		//If we're still running fork else exit
		if(flags[QUIT_FLAG]) {
			exit(0);
		} else if(flags[BACKGROUND_FLAG]) {
			//If background, ignore the child signal
			signal(SIGCHLD, SIG_IGN);
		} else {
			//If not background set child signal to default (incase background has been called previously)
			signal(SIGCHLD, SIG_DFL);
		}

		if(flags[MURDER_FLAG]) {
			murder(args);
			continue;
		} else if(flags[HELP_FLAG]) {
			helpPrompt();
			continue;
		} else if(flags[SCRIPT_FLAG]) {
			if(script()) {
				pID = fork();
			}
		} else if(flags[RUN_FLAG] || flags[BACKGROUND_FLAG]){
			pID = fork();
		}

		if(pID == CHILD_PID) {
			if(flags[RUN_FLAG]) {
				run(args);
			} else if(flags[BACKGROUND_FLAG]) {
				background(args);
			} else if(flags[SCRIPT_FLAG]) {
				const char* a[] = {NULL, "/bin/bash", "t", NULL};
				run ((char**)a);
			}
		} else if(pID < 0) {
			//Failed to fork, print error message and exit program
			fprintf(stderr, "Failed to fork, exiting...\n");
			exit(1);
		} else {
			if(flags[RUN_FLAG] | flags[SCRIPT_FLAG]) {
				//Wait for child if command was run
				waitpid(pID, NULL, 0);
			} else if(flags[BACKGROUND_FLAG]) {
				//Sleep for a second so child process can print PID (Makes output look nicer)
				sleep(1);
			}

			if(flags[SCRIPT_FLAG]) {
				if(remove("t") != 0) {
					fprintf(stderr, "Error deleting temp script file\n");
				}
			}
		}
	}
}


/*	
	in: 	Pointer to the line of input
	args: 	String array to place arguments
*/
void getArgs(char* in, char** args) {
	//Working pointer to args
	char** arg;
	arg = args;

	//Get the first token and continue through string placing 
	//tokens in args, last token will be NULL
	*arg++ = strtok(in, DELIMITERS);
	while((*arg++ = strtok(NULL, DELIMITERS)));
}


/*
	args	String array storing arguments
*/	
void run(char** args) {
	//Working pointer to args
	char** arg;

	//Set arg and throw away 'run'
	arg = args;
	*arg++;

	//Get the path to the executable
	char* path = *arg;
	fprintf(stdout, "Executing: ");
	printArray(arg);
	//Execute!
	int r = execv(path, arg);

	//If this code is reached execv has returned instead of exited, print error and exit program
	fprintf(stderr, "Error: %d occured\n", errno);
	exit(1);
}


/*
	args	String array storing arguments
*/
void background(char** args) {
	fprintf(stdout, "PID = %d\n", getpid());

	//Reopen std IO's don't want to print to console anymore
	freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

    //Run the command
    run(args);
} 


/*
	args	String array storing arguments
*/
void murder(char** args) {
	//Working pointer to args
	char** arg;
	arg = args;

	//Throw away the first argument (should be the command)
	*arg++;

	//Get the pid string and convert it to an integer
	char* numstr = *arg;
	int pid = atoi(numstr);
	
	//Try and kill the process
	switch(kill(pid, SIGTERM)) {
		case -1:
			fprintf(stderr, "PID %d not terminated error %d \n", pid, errno);
			break;
		default:
			fprintf(stdout, "Successfully killed process %d\n", pid);
			break;
	}
}


/*
	args: 	String array to check for quit command
*/
void inCheck(char** args) {
	//Working pointer to args
	char** arg;	
	arg = args;

	//Reset flags
	flushFlags();

	//If there was an argument, check for the command
	if(*arg) {
		if(!strcmp(*arg, QUIT)) {
			flags[QUIT_FLAG] = 1;
		} else if(!strcmp(*arg, HELP)) {
			flags[HELP_FLAG] = 1;
		} else if(!strcmp(*arg, RUN)) {
			flags[RUN_FLAG] = 1;
		} else if(!strcmp(*arg, BACKGROUND)) {
			flags[BACKGROUND_FLAG] = 1;
		} else if(!strcmp(*arg, MURDER)) {
			flags[MURDER_FLAG] = 1;
		} else if(!strcmp(*arg, SCRIPT)) {
			flags[SCRIPT_FLAG] = 1;
		} else {
			fprintf(stdout, "Invalid input use help for commands\n");
		}
	} 
}

int script() {
	fprintf(stdout, "Type your script in bash syntax and use :q on a newline to quit\n");
	fprintf(stdout, "Use :qr to quit and run the script, without saving to file\n");	
	fprintf(stdout, "Use :qs [filename] to quit, run the script and save to filename\n");
	fprintf(stdout, "NOTE: (important) This module saves the bash file to filename=t,\n");
	fprintf(stdout, "make sure you dont have any files named t in this directory!\n");

	FILE* tempbash = fopen("t", "w");

	if(tempbash == NULL) {
		fprintf(stderr, "Script Error exiting\n");
		return 0;
	}

	char buf[BUFFER_SIZE];
	char temp[BUFFER_SIZE];
	char bash[BUFFER_SIZE*8];
	char* args[MAX_ARGS];
	int i, b;

	b = 0;
	clearBuffer(bash);

	while(true) {
		clearBuffer(buf);
		clearBuffer(temp);
		fgets(buf, BUFFER_SIZE, stdin);
		
		//copy to a temp buffer so that strtok does not modify buf
		for(i = 0; i < BUFFER_SIZE; i++) {
			temp[i] = buf[i];
		}

		getArgs(temp, args);

		//check for exit situations
		if(!strcmp(args[0], ":q")) {
			//Just quit, close tempbash and exit 0
			fclose(tempbash);
			return 0;
		} else if(!strcmp(args[0], ":qs")) {
			

			//Try and get the userfilename
			char* userFilename;
			if((userFilename = args[1])) {
				//Weird things happen if you dont remove it first
				remove(userFilename);

				//Open the userfile and write to approprate locations
				FILE* userFile = fopen(userFilename, "w");
				fprintf(tempbash, "%s", bash);
				fprintf(userFile, "%s", bash);

				//cleanup
				fclose(userFile);
				fclose(tempbash);
				return 1;
			} else {
				//No userfile found just print to the tempfile
				fprintf(stderr, "Invalid user Specified filename, not saving\n");
				fprintf(tempbash, "%s", bash);
				fclose(tempbash);
				return 1;
			}
		} else if(!strcmp(args[0], ":qr")) {
			//Quit and run, print to tempfile and return 1
			fputs(bash, tempbash);
			fclose(tempbash);
			return 1;
		}

		//Write to the running bash array
		for(i = 0; i < BUFFER_SIZE; i++) {
			if(b < BUFFER_SIZE*8) {
				bash[b] = buf[i];
				b++;
			} else {
				fprintf(stderr, "Bash buffer full, exiting\n");
				return 0;
			}
		}
	}
}

void clearBuffer(char buf[]) {
	memset(buf, '\0', sizeof(buf));
}


void helpPrompt() {
	fprintf(stdout, "%s", HELP_PROMPT);
}


void prompt() {
	fprintf(stdout, PROMPT);
}


void flushFlags() {
	int i;

	for(i = 0; i < MAX_FLAGS; i++) {
		flags[i] = 0;
	}
}

void printArray(char** a) {
	char** b;
	b = a;

	while (*b) {
		char* s = *b++;
		fprintf(stdout, "%s ", s);
	}
	fprintf(stdout, "\n");
}