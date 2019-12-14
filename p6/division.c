////////////////////////////////////////////////////////////////////////////////
// Main File:        division.c
// This File:        division.c
// Other Files:      intdate.c sendsig.c
// Semester:         CS 354 Fall 2019
//
// Author:           Matthew Chiang
// Email:            mchiang7@wisc.edu
// CS Login:         mchiang
//
//////////////////////////// 80 columns wide ///////////////////////////////////

#include <stdlib.h> 
#include <stdio.h> 
#include <signal.h> 
#include <unistd.h> 
#include <time.h> 
#include <string.h> 
#include <sys/types.h> 

int successes = 0; // counts total number of division successes

// handles SIGINT and reports end program results
void sigint_handler(int signum){
	printf("total number of operations successfully completed: %i\n", successes);
	printf("the program will be terminated.\n");
	exit(EXIT_SUCCESS);
}

// handles SIGFPE and reports end program results
void sigfpe_handler(int signum){
	printf("Error: a division by 0 operation was attempted.\n");	
	printf("total number of operations successfully completed: %i\n", successes);
	printf("the program will be terminated.\n");
	exit(EXIT_SUCCESS);
}

// Wrapper function for handling error message formatting
void unix_error(char* error){
	fprintf(stderr, "%s\n", error);
	exit(EXIT_FAILURE);
}

int main(){
	
	// sets up signal handler for SIGINT
	struct sigaction kill;
	memset(&kill, 0, sizeof(kill));
	kill.sa_handler = sigint_handler;
	if (sigaction(SIGINT, &kill, NULL) < 0)
      unix_error("Signal Error: Unable to handle SIGINT");	

	// sets up signal handler for SIGFPE
	struct sigaction divisionErr;
	memset(&divisionErr, 0, sizeof(divisionErr));
	divisionErr.sa_handler = sigint_handler;
	if (sigaction(SIGFPE, &divisionErr, NULL) < 0)
      unix_error("Signal Error: Unable to handle SIGFPE");	
	

	// infinite loop to repeatedly get user input
	while(1){
		// declares integers for division
		int first, second;	

		// prompts and asks for first integer
		printf("Enter first integer: ");
		char buffer [100];
		fgets(buffer, 100, stdin);
		first = atoi(buffer);
	
		// prompts and asks for second integer
		printf("Enter second integer: ");
		fgets(buffer, 100, stdin);
		second = atoi(buffer);		


		// calcualte divsion and remainder of first and second
		int division = first/second;
		int remainder = first%second;
		
		//prints out info of current division and increments successes
		printf("%i / %i is %i with a remainder of %i\n", first, second, division, remainder);
		successes++;
	}
	
	return(0);
}
