/*
	Kenan Krijestorac
	Project 6 - Resource Management
	Due: 10 May 2021
	*Submitted as Makeup Project - 15 May 2021*
*/

#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <time.h>
#include <string.h>

//HEADER FILES
#include "shared.h"
#include "queue.h"

//MACROS FOR CAPS OF PROCESSES; TIMER; FILE OUTPUT
#define MAX_SYSTEM_PROCESS 18
#define HARD_CAP 40
#define SECONDS 5
#define NUM_LINES_CAP 100000

//VARIABLES FOR SHARED MEMORY; FILE POINTER; MSG Q
SharedMemory * shared = NULL;
FILE * fp = NULL;
Message msg;

//MESSAGE QUEUE IDS
static int pMsgQID;
static int cMsgQID;

//FUNCTION PROTOTYPES
void allocation();
void signalHandler(int);
void setTimer(int);
void logCheck(char*);
void usage();

int main(int argc, char* argv[]) {

	//singal handler for timer and ctrl+c
	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);
	srand(time(NULL));
	
	//getopt variables
	int opt;
	int v;
	int seconds = SECONDS;
	
	bool verbose = false;					//FLAG USED TO TRACK IF VERBOSE MODE IS SET
	bool numLinesCap = false;				//FLAG USED TO TRACK WHEN TO STOP WRITING TO LOG FILE
	
	while((opt = getopt(argc, argv, "hv:s:")) != -1) {
		switch(opt) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case 'v':
				if((v = atoi(optarg)) < 0) {
					perror("oss.c: error: invalid input for option -v");
					usage();
					exit(EXIT_SUCCESS);
				}
				
				if(v == 1) {
					verbose = true;
				}
				else if(v == 0) {
					verbose = false;
				}	
				break;
			case 's':
				if((seconds = atoi(optarg)) < 0) {
					perror("oss.c: error: invalid input for timer; seconds > 0");
					usage();
					exit(EXIT_SUCCESS);
				}
				break;
			default:
				perror("oss.c: error: invalid argument");
				usage();
				exit(EXIT_FAILURE);
		}
	}

	printf("==============================================\n");
	printf("\t\tRESOURCE MANAGER\n");
	printf("==============================================\n");
	printf("%s\n", verbose ? "true" : "false");
	printf("seconds: %d\n", seconds);
	

	execl("./user_proc", "user_proc", (char*)NULL);

	return EXIT_SUCCESS;


}

//SETS-UP SHARED MEMORY AND MESSAGE QUEUES
void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
}

//BASIC SIGNAL HANDLER TO CLEAN UP
void signalHandler(int signal) {
	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();

	if(signal == SIGINT) {
		printf("oss.c: terminating: ctrl + c signal handler\n");
		fflush(stdout);
	}
	else if(signal == SIGALRM) {
		printf("oss.c: terminating: timer signal handler\n");
		fflush(stdout);
	}
	
	while(wait(NULL) > 0);
	exit(EXIT_SUCCESS);
}

//BASIC TIMER TAKEN FROM BOOK; USED ON PREVIOUS PROJECTS
void setTimer(int seconds) {
	struct sigaction act;
	act.sa_handler = &signalHandler;
	act.sa_flags = SA_RESTART;
	
	if(sigaction(SIGALRM, &act, NULL) == -1) {
		perror("oss.c: error: failed to set up sigaction handler for setTimer()");
		exit(EXIT_FAILURE);
	}
	
	struct itimerval value;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	
	if(setitimer(ITIMER_REAL, &value, NULL)) {
		perror("oss.c: error: failed to set the timer");
		exit(EXIT_FAILURE);
	}
}

//CHECKS TO SEE IF LOG FILE CAN BE OPENED
void logCheck(char * file) {
	fp = fopen(file, "w");

	if(fp == NULL) {
		perror("oss.c: error: failed to open log file");
		exit(EXIT_FAILURE);
	}
}

//BASIC USAGE MENU; HOW TO RUN, ETC
void usage() {
	printf("=================================================================================\n");
        printf("\t\t\t\tUSAGE\n");
        printf("=================================================================================\n");
        printf("./oss -h [-s t] [-v f]\n");
        printf("run as: ./oss [options]\n");
        printf("\tex. : ./oss -s 5 -v 1\n");
        printf("\tif [-s t] is not defined it is set as default 5 seconds\n");
        printf("\tvariable for verbose should be set to 1 or 0\n");
        printf("=================================================================================\n");
        printf("-h		:	Describe how project should be run and then, terminate\n");
        printf("-s t		:	Indicate the number of seconds for timer\n");       
        printf("-v f		:	Indicates if verbose is set; 1 for \"on\" and 0 for \"off\"\n");
        printf("=================================================================================\n");
}
