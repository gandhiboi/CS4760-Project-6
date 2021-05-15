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
#define NUM_RESOURCES 20
#define HARD_PROCESS_CAP 40
#define SECONDS 5
#define NUM_LINES_CAP 100000

//VARIABLES FOR SHARED MEMORY; FILE POINTER; MSG Q
SharedMemory * shared = NULL;
FILE * fp = NULL;
Message msg;
Queue * waitQ;

int pids[MAX_SYSTEM_PROCESS];
int processCounter = 0;

//MESSAGE QUEUE IDS
static int pMsgQID;
static int cMsgQID;

//FUNCTION PROTOTYPES
void resourceManager(bool);
void requestResource();
void releaseResource();
void terminateResource();
void spawnUser();
void initDescriptors();
void chooseSharedResources();
void incrementSimClock(SimulatedClock *, int);
void allocation();
void signalHandler(int);
void setTimer(int);
void logCheck(char*);
void usage();

int main(int argc, char* argv[]) {

	//SIGNAL HANDLER FOR TIMER AND CTRL+C
	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);

	srand(time(NULL));
	
	//GETOPT VARIABLES
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
	
	allocation();
	initDescriptors();
	resourceManager(verbose);
	
	sleep(3);
	
	releaseSharedMemory();
	deleteSharedMemory();
	deleteMessageQueues();

	return EXIT_SUCCESS;


}

void resourceManager(bool v) {

	int incrementTime = (rand() % 500000000 - 1000000) + 1000000;

	while(1) {
	
		msgrcv(cMsgQID, &msg, sizeof(msg), 0, IPC_NOWAIT);
		
		if(strcmp(msg.mtext, "REQUEST") == 0) {
		
		}
		else if(strcmp(msg.mtext, "RELEASE") == 0) {
		
		}
		else if(strcmp(msg.mtext, "TERMINATE") == 0) {
		
		}
		else {
			perror("oss.c: error: no valuable message received from user; terminating program");
			exit(EXIT_FAILURE);
		}
	
		incrementSimClock(&(shared->simClock), incrementTime);
		printf("%d:%d\n", shared->simClock.sec, shared->simClock.ns);
	
	}

}



void requestResource() {

}

void releaseResource() {

}

void terminateResource() {

	

}

//INCREMENTS SYSTEM SIM CLOCK; CONVERTS NS TO SEC IF WRAPS
void incrementSimClock(SimulatedClock* timeStruct, int increment) {
	int nanoSec = timeStruct->ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct->sec)++;
	}
	timeStruct->ns = nanoSec;
}

//INITIALIZE VECTORS FOR RESOURCE DESCRIPTORS
void initDescriptors() {
	int i;
	int j;
	for(i = 0; i < NUM_RESOURCES; i++) {
		int randNumInstances = (rand() % 10) + 1;
		shared->resource[i].numInstancesAvailable = randNumInstances;
		shared->resource[i].usedInstances = randNumInstances;
		shared->resource[i].sharedResourceFlag = 0;
		for(j = 0; j < 18; j++) {						//should be j <= 18
			shared->resource[i].allocated[j] = 0;
			shared->resource[i].release[j] = 0;
			shared->resource[i].request[j] = 0;
		}
	}
	chooseSharedResources();
}

void chooseSharedResources() {
	int randSharedResources = (rand() % 5) + 1;
	
	int i;
	int randSharedLocation;
	for(i = 0; i < randSharedResources; i++) {
		randSharedLocation = (rand() % 20);
		shared->resource[randSharedLocation].sharedResourceFlag = 1;
	}
}

//SPAWNS CHILD PROCESS AND CHECKS IF ABLE TO SPAWN
void spawnUser() {

	pid_t pid = fork();

	if(pid == 0) {
		execl("./user_proc", "user_proc", (char*)NULL);
		exit(EXIT_SUCCESS);
	}

}

//SETS-UP SHARED MEMORY AND MESSAGE QUEUES
void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
	
	waitQ = createQueue(MAX_SYSTEM_PROCESS);
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
