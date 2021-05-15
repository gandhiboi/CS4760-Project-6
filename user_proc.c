#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/msg.h>

#include "shared.h"

//MACROS FOR PERCENTAGES AND BOUND 'B'
#define PERCENT_REQUEST 70
#define PERCENT_TERMINATE 10
#define BOUND 1000000

//VARIABLES/OBJECTS FOR SHARED MEMORY, MESSAGE QUEUES; TIME STRUCTURES
SharedMemory * shared = NULL;
Message msg;
SimulatedClock terminateClock = {0,0};
SimulatedClock boundClock = {0,0};

//MESSAGE QUEUE IDS
static int pMsgQID;
static int cMsgQID;

//FUNCTION PROTOTYPES
void allocation();
void requestOrRelease(int);
void termination(int);
SimulatedClock incrementLocalClock(SimulatedClock, int);

int main(int argc, char * argv[]) {

	//ATTACHES TO SHARED MEMORY AND SEEDS WITH PID
	srand(getpid());
	allocation();
	
	//INDEX FROM VECTORPID IN OSS; USED FOR ACCESSING PROPER RESOURCE DESCRIPTORS/ACTIONS
	int index = atoi(argv[0]);
	
	//GENERATES RANDOM CHECK FOR TERMINATION [1-250]MS AND [0,B]
	int checkTerminate = (rand() % 250000000) + 1;
	int requestOrReleaseCheck = (rand() % BOUND) + 1;
	
	//SETS LOCAL CLOCKS TO CURRENT SHARED MEMORY CLOCK
	terminateClock.sec = shared->simClock.sec;
	terminateClock.ns = shared->simClock.ns;
	boundClock.sec = shared->simClock.sec;
	boundClock.ns = shared->simClock.ns;
	
	//ADDS THE RANDOMLY GENERATED VALUES TO LOCAL CLOCKS TO BE COMPARED TO SIM SYSTEM CLOCK
	terminateClock = incrementLocalClock(terminateClock, checkTerminate);
	boundClock = incrementLocalClock(boundClock, requestOrReleaseCheck);

	//
	while(1) {
		//SHOULD A RELEASE OR REQUEST BE MADE
		if((shared->simClock.sec >= boundClock.sec) || ((shared->simClock.sec == boundClock.sec) && (shared->simClock.ns >= boundClock.ns))) {
			boundClock.sec = shared->simClock.sec;
			boundClock.ns = shared->simClock.ns;
			boundClock = incrementLocalClock(boundClock, requestOrReleaseCheck);
			requestOrRelease(index);		
		}
		
		//CHECK IF PROCESS SHOULD BE TERMINATED; IF NOT THEN REROLL FOR NEW TERMINATION TIME
		if((shared->simClock.sec >= terminateClock.sec) || ((shared->simClock.sec == terminateClock.sec) && (shared->simClock.ns >= terminateClock.ns))) {
			checkTerminate = (rand() % 250000000) + 1;
			terminateClock.sec = shared->simClock.sec;
			terminateClock.ns = shared->simClock.ns;
			terminateClock = incrementLocalClock(terminateClock, checkTerminate);
			termination(index);
		}
	}

	return EXIT_SUCCESS;

}

void requestOrRelease(int pid) {

	//GENERATE RANDOM PERCENTAGE TO REQUEST RESOURCE OTHERWISE RELEASE
	int randReq = (rand() % 100);
	
	if(randReq < PERCENT_REQUEST) {
		//SEND REQUEST MESSAGE TO OSS
		strcpy(msg.mtext, "REQUEST");
		msg.mtype = pid;
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);
		
		//SELECT WHICH RESOURCE TO REQUEST TO OSS
		int requestResource = (rand() % 20);
		sprintf(msg.mtext, "%d", requestResource);
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);
		
		//RANDOMLY SELECT NUMBER OF INSTANCES TO REQUEST FROM OSS
		int selectInstance = (rand() % shared->resource[requestResource].numInstancesAvailable) + 1;
		sprintf(msg.mtext, "%d", selectInstance);
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);
		
		//LOOP/BLOCK PROCESS UNTIL MESSAGE IS RECEIVED TO GRANT RESOURCE OR TERMINATE
		while(1) {
			//SENDS MESSAGE TO OSS IF RESOURCE HAS BEEN GRANTED
			msgrcv(pMsgQID, &msg, sizeof(msg), pid, 0);
			if(strcmp(msg.mtext, "GRANT") == 0) {
				break;
			}
			//SENDS MESSAGE TO OSS IF RESOURCE NEEDS TO TERMINATE
			else if(strcmp(msg.mtext, "END") == 0) {
				exit(EXIT_SUCCESS);
			}
		}
	}
	else {
		//SEND RESOURCE RELEASE MESSAGE TO OSS
		strcpy(msg.mtext, "RELEASE");
		msg.mtype = pid;
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);
		
		int i;
		int releaseResource;
		for(i = 0; i < 20; i++) {
			if(shared->resource[i].allocated[pid-1] > 0) {
				releaseResource = i;
			}
		}
		//SENDS MESSAGE TO OSS WHICH RESOURCE TO RELEASE
		sprintf(msg.mtext, "%d", releaseResource);
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);	
	}
}

//RESPONSIBLE FOR TERMINATING PROCESS
void termination(int pid) {

	//GENERATE RANDOM PERCENTAGE TO TERMINATE
	int randTerminate = (rand() % 100);

	//SEND MESSAGE TO OSS FOR TERMINATION
	if(randTerminate <= PERCENT_TERMINATE) {
		strcpy(msg.mtext, "TERMINATE");
		msg.mtype = pid;
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);
		exit(EXIT_SUCCESS);
	}

}

//INCREMENTS THE LOCAL CLOCKS FOR RUNNING RESPECTIVE LOOP
SimulatedClock incrementLocalClock(SimulatedClock timeStruct, int increment) {
	int nanoSec = timeStruct.ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct.sec)++;
	}
	timeStruct.ns = nanoSec;
	
	return timeStruct;
}

//ATTACHES TO SHARED MEMORY
void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
}

