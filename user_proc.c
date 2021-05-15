#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/msg.h>

#include "shared.h"

#define PERCENT_REQUEST 70
#define PERCENT_TERMIANTE 10
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
void requestOrRelease(int, int);
void termination(int, int);
SimulatedClock incrementSimClock(SimulatedClock, int);

int main(int argc, char * argv[]) {

	srand(getpid());
	allocation();
	
	int checkTerminate = (rand() % 250000000) + 1;
	int requestOrReleaseCheck = (rand() % BOUND) + 1;
	
	printf("user: checkTerminate: %d\n", checkTerminate);
	printf("user: requestOrReleaseCheck: %d\n", requestOrReleaseCheck);
	
	terminateClock.sec = shared->simClock.sec;
	terminateClock.ns = shared->simClock.ns;
	boundClock.sec = shared->simClock.sec;
	boundClock.ns = shared->simClock.ns;
	
	terminateClock = incrementSimClock(terminateClock, checkTerminate);
	boundClock = incrementSimClock(boundClock, requestOrReleaseCheck);
	
	printf("user: terminateClock: %d:%d\n", terminateClock.sec, terminateClock.ns);
	printf("user: boundClock: %d:%d\n", boundClock.sec, boundClock.ns);

	printf("user: hello world\n");

	while(1) {
	
	}

	return EXIT_SUCCESS;

}

void requestOrRelease(int reqRelCheck, int pid) {

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
		
	}
	else {
		//SEND RESOURCE RELEASE MESSAGE TO OSS
		strcpy(msg.mtext, "RELEASE");
		msg.mtype = pid;
		msgsnd(cMsgQID, &msg, sizeof(msg), 0);
		
		
	}
	

}

void termination(int termCheck, int pid) {

}

SimulatedClock incrementSimClock(SimulatedClock timeStruct, int increment) {
	int nanoSec = timeStruct.ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct.sec)++;
	}
	timeStruct.ns = nanoSec;
	
	return timeStruct;
}

void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
}

