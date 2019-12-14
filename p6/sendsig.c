////////////////////////////////////////////////////////////////////////////////
// Main File:        sendsig.c
// This File:        sendsig.c
// Other Files:      intdate.c division.c
// Semester:         CS 354 Fall 2019
//
// Author:           Matthew Chiang
// Email:            mchiang7@wisc.edu
// CS Login:         mchiang
//
//////////////////////////// 80 columns wide ///////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

// sends the process with id pid the SIGINT signal
void send_sigint(int pid){
	if(kill(pid, SIGINT) < 0){
  	printf("SIGINT unable to send\n");
		exit(0);
	}
}

// sends the process with id pid the SIGUSR1 signal
void send_sigusr1(int pid){
	if(kill(pid, SIGUSR1) < 0){
		printf("SIGUSR1 unable to send\n");
		exit(0);
	}
}

// checks if the string is a number
int isnumber(char* string){
	char curr_char;
	int i = 0;
	do{
		curr_char = string[i];
		if(!isdigit(curr_char))
			return isdigit(curr_char);
	}while (string[++i] != '\0');
	return 1;
}

void main(int argc, char *argv[]){
	// checking for correct number of command line args
	if(argc != 3){
		printf("Usage: <signal type> <pid>\n");
		exit(0);
	}

	// getting the signal type
	char *signal = argv[1];
	char signal_t = signal[1];
	
	// checking for correct signal type
	if(signal_t != 'u' && signal_t != 'i'){ // must be u or i
		printf("Usage: <signal type> <pid>\n");
    exit(0);
  }

	// getting user input for pid
	char* raw_pid = argv[2];
	
	// checking to make sure user input is valid
	if(!isnumber(raw_pid)){
		printf("Usage: <signal type> <pid>\n");
		exit(0);
	}
	
	// converting raw pid into an int
	int pid = atoi(raw_pid);

	// determinds which signal to send
	if(signal_t == 'u')
		send_sigusr1(pid); // wrapper function call to send SIGUSR1 to PID

	if(signal_t == 'i')
		send_sigint(pid); // wrapper function call to send SIGINT to PID
}
