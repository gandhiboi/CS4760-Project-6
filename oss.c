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

int pidVector[MAX_SYSTEM_PROCESS];
int pids[MAX_SYSTEM_PROCESS];
int processCounter;

//MESSAGE QUEUE IDS
static int pMsgQID;
static int cMsgQID;

//STATISTICS
double percentDeadlock;
int numTerminateDeadlock;
int numTerminateSuccess;
int numRequestImmediate;
int numRequestWait;
int numDeadlockDetection;
int numTerminateTotal;
int grantedResources;
bool tableFlag;
int running;

//FUNCTION PROTOTYPES
void resourceManager(bool);
void spawnUser(int);
void initDescriptors();
void chooseSharedResources();
void incrementSimClock(SimulatedClock *, int);
void allocation();
void signalHandler(int);
void setTimer(int);
void logCheck(char*);
void usage();
int findIndex();
void releaseResources(int, int);
int accessResource(int, int);
void checkForResources(int);

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
	char * fileName = "output.log";
	
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
	printf("Verbose State: %s\n", verbose ? "true" : "false");
	printf("Timer: %d seconds\n", seconds);
	printf("==============================================\n");
	printf("\t    SIMULATION IN PROGRESS\n");
	
	//CHECK TO SEE IF FILE CAN BE OPENED
	logCheck(fileName);
	
	//TIMER VALUE SET HIGHER BECAUSE OF ISSUES
	setTimer(10);
	
	allocation();
	initDescriptors();
	resourceManager(verbose);
	
	
	printf("==============================================\n");
	printf("\t    STATISTICS\n");
	printf("Processes Terminated Due to Deadlock: %d\n", numTerminateDeadlock);
	printf("Processes Terminated Successfully: %d\n", numTerminateSuccess);
	printf("Requests Granted Immediately: %d\n", numRequestImmediate);
	printf("Requested Granted After Wait: %d\n", numRequestWait);
	printf("Number of Times Deadlock Detection Ran: %d\n", numDeadlockDetection);
	printf("Total Number of Process Terminations: %d\n", numTerminateTotal);
	
	releaseSharedMemory();
	deleteSharedMemory();
	deleteMessageQueues();

	return EXIT_SUCCESS;


}

void resourceManager(bool v) {

	//SET SECONDS TO 1 SO IT CAN BE USED TO RUN DETECTION ALGORITHM EVERY ONE SECOND
	SimulatedClock checkForDeadlock = {1 , 0};
	running = 0;									//NUMBER OF CURRENT RUNNING PROCESSES
	pid_t pids[MAX_SYSTEM_PROCESS];
	
	//INITIALIZES PIDS TO 0
	int q;
	for(q = 0; q < MAX_SYSTEM_PROCESS; q++) {
		pids[q] = 0;
	}

	while(1) {
		//GET CLOCK GOING AND INCREMENT A LITTLE IF MAX SYSTEM PROCESSES IS REACHED
		incrementSimClock(&(shared->simClock), 10000);
		
		//EXIT LOOP IF NUMBER OF PROCESS IS > 40
		if(processCounter > 40) {
			break;
		}
		
		//CREATE NEW PROCESS IF AMOUNT OF RUNNING PROCESS < 18 AND TOTAL PROCESSES < 40
		if((running < MAX_SYSTEM_PROCESS) && (processCounter < 40)) {
			//CREATE NEW PROCESS
			int generateProcessTime = (rand() % (500000000 - 1000000 + 1)) + 1000000;
			incrementSimClock(&(shared->simClock), generateProcessTime);
				
			//FINDS AN EMPTY INDEX TO DETERMINE IF YOU SHOULD SPAWN A CHILD
			int index = findIndex() + 1;
			if((index - 1) > -1) {
				pids[index - 1] = fork();
				if(pids[index - 1] == 0) {
					int length = snprintf(NULL, 0, "%d", index);
					char * xx = (char*)malloc(length + 1);
					snprintf(xx, length + 1, "%d", index);
					execl("./user_proc", xx, (char*)NULL);			//SENDS PID TO USER PROCESS FOR COMMUNICATION BETWEEN MESSAGE QUEUES
					free(xx);
					xx = NULL;
					exit(0);
				}
				fprintf(fp, "OSS: Process P%d created at time [%d:%d]\n", index, shared->simClock.sec, shared->simClock.ns);
				running++;
			}
		}
			
		//RECEIVES THE FIRST MESSAGE FROM THE USER PROCESS AND COMPLETES RESPECTIVE TAKS 
		if(msgrcv(cMsgQID, &msg, sizeof(msg), 0, IPC_NOWAIT) > -1) {
			//msgrcv(cMsgQID, &msg, sizeof(msg), 0, IPC_NOWAIT);
			int pid = msg.mtype;									
			
			//RECEIVE MESSAGE FROM CHILD PROCESS TO REQUEST RESOURCES
			if(strcmp(msg.mtext, "REQUEST") == 0) { 
				//RECEIVE MESSAGE FOR WHICH RESOURCES TO REQUEST
				msgrcv(cMsgQID, &msg, sizeof(msg), pid, 0);
				int requested = atoi(msg.mtext);
				if(v == 1) {
					fprintf(fp, "OSS: Detected Process P%d requesting R%d at time [%d:%d]\n", pid, requested, shared->simClock.sec, shared->simClock.ns);
				}
				
				//RECEIVE MESSAGE FOR NUMBER OF INSTANCES OF RESOURCE R TO REQUEST
				msgrcv(cMsgQID, &msg, sizeof(msg), pid, 0);
				int instances = atoi(msg.mtext);
				if(v == 1) {
					fprintf(fp, "OSS: Process P%d requesting %d instances of R%d\n", pid, instances, requested);
				}
				shared->resource[requested].request[pid - 1] = instances;
				
				//CHECKS TO SEE IF RESOURCE CAN BE GRANTED
				if(accessResource(requested, pid) == 1) {
					grantedResources++;
					if(grantedResources % NUM_RESOURCES == 0) {
						tableFlag = true;
					}
					strcpy(msg.mtext, "GRANT");
					msg.mtype = pid;
					msgsnd(pMsgQID, &msg, sizeof(msg), IPC_NOWAIT);
					numRequestImmediate++;
					if(v == 1) {
						fprintf(fp, "OSS: Granting P%d request R%d at time [%d:%d]\n", pid, requested, shared->simClock.sec, shared->simClock.ns);
					}
				}
				//IF RESOURCE CANNOT BE GRANTED THEN PUT BACK IN WAIT QUEUE
				else {
					if(v == 1) {
						fprintf(fp, "OSS: No instances of R%d available, P%d added to wait queue at time [%d:%d]\n", requested, pid, shared->simClock.sec, shared->simClock.ns);
					}
					enqueue(waitQ, pid);
					numRequestWait++;
				} 

			}
			//RECEIVES MESSAGE FROM CHILD PROCESS TO RELEASE RESOURCES
			else if(strcmp(msg.mtext, "RELEASE") == 0) {
			
				msgrcv(cMsgQID, &msg, sizeof(msg), pid, 0);
				int released = atoi(msg.mtext);
				if(released > -1) {
					releaseResources(released, pid);
					if(v == 1) {
						fprintf(fp, "OSS: Acknowledged Process P%d releasing R%d at time [%d:%d]\n", pid, released, shared->simClock.sec, shared->simClock.ns);
					}
				}
		
			}
			//RECEIVES MESSAGE FROM CHILD PROCESS TO TERMINATE PROCESS
			//WAITS FOR PROCESS TO FINISH UP AND RESETS THE RESOURCE DESCRIPTORS TO DEFAULT TO FREE FOR OTHER PROCESSES
			else if(strcmp(msg.mtext, "TERMINATE") == 0) {
				while(waitpid(pids[pid - 1], NULL, 0) > 0);
				int m;
				numTerminateSuccess++;
				for(m = 0; m < NUM_RESOURCES; m++) {
					if(shared->resource[m].sharedResourceFlag == 0) {
						if(shared->resource[m].allocated[pid - 1] > 0) {
							shared->resource[m].usedInstances += shared->resource[m].allocated[pid - 1];
						}
					}
					shared->resource[m].allocated[pid - 1] = 0;
					shared->resource[m].request[pid - 1] = 0;
				}
				running--;
				pidVector[pid - 1] = 0;
				fprintf(fp, "OSS: Process P%d has terminated at time [%d:%d]", pid, shared->simClock.sec, shared->simClock.ns);
			}
			else {
				perror("oss.c: error: no valuable message received from user; terminating program");
				//exit(EXIT_FAILURE);
			}
		}
		
		int i = 0;
		if(isEmpty(waitQ) == 0) {
			int size = sizeQ(waitQ);
			while(i < size) {
				int pid = dequeue(waitQ);
				int requested;
				//LOOPS THROUGH RESOURCES PROCESS TO SEE WHICH PROCESS WAS REQUESTED/AMOUNT OF AVAILABLE
				int m;
				for(m = 0; m < NUM_RESOURCES; m++) {
					if(shared->resource[m].request[pid - 1] > 0) {
						requested = m;
					}
				}
				//CHECKS TO SEE IF RESOURCES CAN BE GIVEN TO PROCESS
				if(accessResource(requested, pid) == 1) {
					grantedResources++;
					if((grantedResources % NUM_RESOURCES) == 0) {
						tableFlag = true;
					}
					if(v == 1) {
						fprintf(fp, "OSS: Granting Process P%d in wait queue Resource %d at time [%d:%d]\n", pid, requested, shared->simClock.sec, shared->simClock.ns);
					}
					//SEND MESSAGE TO PROCESS THAT 
					strcpy(msg.mtext, "GRANT");
					msg.mtype = pid;
					msgsnd(pMsgQID, &msg, sizeof(msg), IPC_NOWAIT);
				}
				else {
					//REQUEUE PROCESS SINCE THE RESOURCE IS NOT AVAILABLE
					enqueue(waitQ, pid);
					numRequestWait++;
				}
				i++;
			}
		}
		
		//RUN DEADLOCK DETECTION ALGORITHM
		if(shared->simClock.sec == checkForDeadlock.sec) {
			bool flag = false;					//FLAG USED TO CHECK IF DEADLOCK NEEDS BE RUN
			numDeadlockDetection++;
		
			//IF THE QUEUE HAS PROCESSES THEN SET FLAG AND RUN DEADLOCK DETECTION
			if(isEmpty(waitQ) == 0) {
				flag = true;
				fprintf(fp, "OSS: Running deadlock detection at time [%d:%d]\n", shared->simClock.sec, shared->simClock.ns);
			}
			//IF QUEUE IS EMPTY THEN SET FLAG AND MOVE ON
			else {
				flag = false;
			}
			
			//DEADLOCK DETECTION IS RUN IF THERE ARE PROCESSES IN WAIT QUEUE
			if(flag == 1) {
				while(1) {
					//SEND MESSAGE TO PROCESS TO END PROCESS THAT IS FIRST IN THE WAIT QUEUE
					flag = false;
					int check = dequeue(waitQ);
					msg.mtype = check;
					strcpy(msg.mtext, "END");
					msgsnd(pMsgQID, &msg, sizeof(msg), IPC_NOWAIT);
					while(waitpid(pids[check - 1], NULL, 0) > 0);				//WAITS FOR PROCESS TO TERMINATE/CLEANUP
					numTerminateDeadlock++;
					
					fprintf(fp, "OSS: Process P%d is deadlocked with other process\n\tTerminating with deadlock algorithm at time [%d:%d]", check, shared->simClock.sec, shared->simClock.ns);
					
					//LOOPS THROUGH TO RESET/RELEASE RESOURCES THAT PROCESS HAD 
					int m;
					for(m = 0; m < NUM_RESOURCES; m++) {
						if(shared->resource[m].sharedResourceFlag == 0) {
							if(shared->resource[m].allocated[check - 1] > 0) {
								shared->resource[m].usedInstances += shared->resource[m].allocated[check - 1];
								if(v == 1) {
									fprintf(fp, "\t\tResources Released: R%d ", m);
								}
							}
						}
						shared->resource[m].allocated[check - 1] = 0;
						shared->resource[m].request[check - 1] = 0;
					}
					running--;
					//LOOP THROUGH ITEMS IN WAIT QUEUE AND GIVE RESOURCES IF THEY ARE AVAVILABLE
					int i = 0;
					while(i < sizeQ(waitQ)) {
						int temp = dequeue(waitQ);
						checkForResources(temp);
						i++;
					}
					//IF QUEUE IS EMPTY THEN LEAVE LOOP; DEADLOCK DETECTION NOT NEEDED
					if(isEmpty(waitQ) != 0) {
						break;
					}
				}
				fprintf(fp, "OSS: Deadlock resolved\n");
			}
		}
		//PRINT RESOURCE TABLE EVERY 20 GRANTED REQUESTS 
		if(grantedResources % NUM_RESOURCES == 0 && tableFlag == 1) {
			int u, v;
			fprintf(fp, "OSS: Printing Resource Table\n\t");
			for(v = 0; v < NUM_RESOURCES; v++) {
				fprintf(fp, "R%d ", v);
			}
			fprintf(fp, "\n");
			for(u = 0; u < MAX_SYSTEM_PROCESS; u++) {
				if(pidVector[u] == 1) {
					fprintf(fp, " P:%d ", u);
					for(v = 0; v < NUM_RESOURCES; v++) {
						fprintf(fp, "%d ", shared->resource[v].allocated[u]);
					}
					fprintf(fp,"\n");
				}
			}
			tableFlag = false;
		}
	
	}
	//AFTER PROCESS LIMIT IS REACHED; WAIT FOR REMAINING PROCESSES TO FINISH
	int status;
	pid_t wpid;
	while((wpid = wait(&status)) > 0);
}

void checkForResources(int pid) {
	int i;
	int requested;
	//LOOPS THROUGH RESOURCES TO CHECK IF INDEX(RESOURCE) IS AVAILABLE
	for(i = 0; i < NUM_RESOURCES; i++) {
		if(shared->resource[i].request[pid - 1] > 0) {
			requested = i;
		}
	}
	
	//CHECKS TO SEE IF R IS AVAILABLE IF SO SO SEND MESSAGE, OTHERWISE PLACE IN WAIT QUEUE AGAIN
	if(accessResource(requested, pid) == 1) {
		fprintf(fp, "OSS: Granting Process P%d request R%d at time [%d:%d]\n", pid, requested, shared->simClock.sec, shared->simClock.ns);
		grantedResources++;
		if(grantedResources % NUM_RESOURCES == 0) {
			tableFlag = true;
		}
		
		//SEND MESSAGE TO USER THAT REQUEST HAS BEEN GRANTED
		strcpy(msg.mtext, "GRANT");
		msg.mtype = pid;
		msgsnd(pMsgQID, &msg, sizeof(msg), IPC_NOWAIT);
	}
	else {
		enqueue(waitQ, pid);					//NO RESOURCES AVAILABLE, REQUEUE
	}

}

//RELEASES RESOURCES
void releaseResources(int resourceId, int pid) {
	if(shared->resource[resourceId].sharedResourceFlag == 0) {
		shared->resource[resourceId].usedInstances += shared->resource[resourceId].allocated[pid - 1];
	}
	shared->resource[resourceId].allocated[pid - 1] = 0;
}

//IF RESOURCE CAN BE GRANTED THEN ADJUST RESOURCE DESCRIPTORS
int accessResource(int resourceId, int pid) {
	while((shared->resource[resourceId].request[pid - 1] > 0 && shared->resource[resourceId].numInstancesAvailable > 0)) {
		//DECREMENTS REQUESTED RESOURCES AND AVAILABLE RESOURCES AND INCREASES ALLOCATED RESOURCES
		if(shared->resource[resourceId].sharedResourceFlag == 0) {
			(shared->resource[resourceId].request[pid - 1])--;
			(shared->resource[resourceId].allocated[pid - 1])++;
			(shared->resource[resourceId].numInstancesAvailable)--;
		}
		else {
			shared->resource[resourceId].request[pid - 1] = 0;
			break;
		}
	}
	
	if(shared->resource[resourceId].request[pid - 1] > 0) {
		return -1;
	}
	else {
		return 1;
	}

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
	int k;
	for(i = 0; i < NUM_RESOURCES; i++) {
		int randNumInstances = (rand() % 10) + 1;
		shared->resource[i].numInstancesAvailable = randNumInstances;
		//printf("oss.c: instances created: %d\n", shared->resource[i].numInstancesAvailable);
		shared->resource[i].usedInstances = randNumInstances;
		shared->resource[i].sharedResourceFlag = 0;
		for(j = 0; j <= MAX_SYSTEM_PROCESS; j++) {						//should be j <= 18
			shared->resource[i].allocated[j] = 0;
			shared->resource[i].release[j] = 0;
			shared->resource[i].request[j] = 0;
		}
	}
	for(k = 0; k < MAX_SYSTEM_PROCESS; k++) {
		pidVector[i] = 0;
	}
	chooseSharedResources();
}

//RANDOMLY DETERMINES WHICH RESOURCES WILL BE SHARED
void chooseSharedResources() {
	int randSharedResources = (rand() % 5) + 1;
	int i;
	int randSharedLocation;
	for(i = 0; i < randSharedResources; i++) {
		randSharedLocation = (rand() % NUM_RESOURCES);
		shared->resource[randSharedLocation].sharedResourceFlag = 0;
	}
}

//SPAWNS CHILD PROCESS AND CHECKS IF ABLE TO SPAWN
void spawnUser(int numProcess) {

	pid_t pid = fork();

	if(pid == 0) {
		execl("./user_proc", "user_proc", (char*)NULL);
		exit(EXIT_SUCCESS);
	}

}

//FINDS AN UNUSED INDEX IN PID VECTOR
int findIndex() {
	int i;
	for(i = 0; i < MAX_SYSTEM_PROCESS; i++) {
		if(pidVector[i] == 0) {
			pidVector[i] = 1;
			return i;
		}
	}
	return -1;
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
	
	printf("==============================================\n");
	printf("\t    STATISTICS\n");
	printf("Processes Terminated Due to Deadlock: %d\n", numTerminateDeadlock);
	printf("Processes Terminated Successfully: %d\n", numTerminateSuccess);
	printf("Requests Granted Immediately: %d\n", numRequestImmediate);
	printf("Requested Granted After Wait: %d\n", numRequestWait);
	printf("Number of Times Deadlock Detection Ran: %d\n", numDeadlockDetection);
	printf("Total Number of Process Terminations: %d\n", numTerminateTotal);

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
