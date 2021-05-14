#include <stdlib.h>
#include <stdio.h>
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
SimulatedClock incrementSimClock(SimulatedClock timeStruct, int increment);

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

	return EXIT_SUCCESS;

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

