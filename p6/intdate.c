////////////////////////////////////////////////////////////////////////////////
// Main File:        intdate.c
// This File:        intdate.c
// Other Files:      division.c sendsig.c
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

int seconds = 3;
int num_user_signals = 0;

void unix_error(char* error);
void sigint_handler(int signum);
void user_signal_handler(int signum);
void signal_handler(int signum);
int main(){
	// instructions to user
	printf("Pid and time will be printed every 3 seconds\n");
	printf("Enter ^C to end the program.\n");
	
	// sets up SIGALRM to go off in seconds seconds
	alarm(seconds); 

	// creates and sets SIGALRM handler
	struct sigaction print_time; // declares sigaction print_time
	memset (&print_time, 0, sizeof(print_time)); 
	print_time.sa_handler = signal_handler; // sets the handler for print_time
	
	// error checking
	if (sigaction(SIGALRM, &print_time, NULL) < 0)
		unix_error("Signal Error: Unable to handle SIGALRM");
	
	// creates and sets SIGUSR1 handler
	struct sigaction user_action; // declares sigaction user_action
	memset (&user_action, 0, sizeof(user_action));
	user_action.sa_handler = user_signal_handler; // sets the handler for user_action 
  
	// error checking
 	if (sigaction(SIGUSR1, &user_action, NULL) < 0)
		unix_error("Signal Error: Unable to handle SIGUSR1");

	// creates and sets SIGINT handler
	struct sigaction end;
	memset (&end, 0, sizeof(end));
	end.sa_handler = sigint_handler;

	// error checking
	if (sigaction(SIGINT, &end, NULL) < 0)
		unix_error("Signal Error: Unable to handle SIGINT");

	// infinite while loop
	while(1){ } 
}

void signal_handler(int signum){//handle the end SIGALRM signal; prints pid and date, re-arms the signal
	// gets the time and PID
	pid_t pid = getpid();

	time_t now;
	if (time(&now) == -1) {
		perror("Error time");
		exit(EXIT_FAILURE);
	}

	// writes out formatted message
	/*
	write(1, "PID: ", sizeof("PID: "));
	write(1, &pid, 4);
	//printf("%i", pid);
	write(1, " | Current time: ", sizeof(" | Current time: "));
	write(1, ctime(&now), 32);
	*/

	printf("PID: %d | Current time: %s", pid, ctime(&now));	

	// redeclares the sigaction struct
	struct sigaction print_time; // declares sigaction print_time
  memset (&print_time, 0, sizeof(print_time));
	print_time.sa_handler = signal_handler; // sets the handler for print_time
  
	// resets the sigaction
  if (sigaction(SIGALRM, &print_time, NULL) < 0)	
		unix_error("Signal Error: Unable to handler SIGALRM");		

	// restarts alarm
	alarm(seconds);
		
}

// signal handler for SIGINT
void sigint_handler(int signum){
	// the following series of writes print out a formatted message to the standard output
	/*
	write(1, "\n", sizeof("\n"));
	write(1, "SIGINT received.", sizeof("SIGINT received."));
	write(1, "\n", sizeof("\n"));
	write(1, "SIGUSR1 was received ", sizeof("SIGUSR1 was received "));
	write(1, &num_user_signals, sizeof(num_user_signals));
	write(1, " times. Exiting now.", sizeof(" times. Exiting now."));
	write(1, "\n", sizeof("\n"));
	*/

	printf("SIGINT received.\nSIGUSR1 was received %d times. Exiting now.\n", num_user_signals);
	exit(EXIT_SUCCESS);
}

// signal handler for SIGUSR1
void user_signal_handler(int signum){
	write(1, "SIGUSR1 caught!", sizeof("SIGUSR1 caught!"));
	write(1, "\n", sizeof("\n"));
	num_user_signals++;
	
	// redeclares the sigaction struct
	struct sigaction action; // declares sigaction print_time
  memset (&action, 0, sizeof(action));
	action.sa_handler = user_signal_handler; // sets the handler for print_time
  
	// resets the sigaction
  if (sigaction(SIGUSR1, &action, NULL) < 0)	
		unix_error("Signal Error: Unable to handle SIGUSR1");		
}


// wrapper function for errors message formatting
void unix_error(char* error){
	fprintf(stderr, "%s\n", error);
	exit(EXIT_FAILURE);
}

