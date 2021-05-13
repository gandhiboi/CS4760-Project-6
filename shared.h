/*
 * Header file for shared memory
 * and message queues
 *
*/

#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>

#define MAX_USER_PROCESS 18
#define NUM_RESOURCES

//Structure for Message Queue
typedef struct {
	long mtype;
	char mtext[100];
} Message;

//Structure for Simulated Clock
typedef struct {
	unsigned int sec;
	unsigned int ns;
} SimulatedClock;

typedef struct {
	int request[MAX_USER_PROCESS];		//resources to be requested
	int release[MAX_USER_PROCESS];		//resources to be released
	int allocated[MAX_USER_PROCESS];		//resources to be allocated
	int sharedResources;				//labels which resources will be shared
	int numberInstances;				//1-10 number of initial instances in each resource class
	int availableInstances;			//instances that have not been consumed yet
} ResourceDescriptors;

typedef struct {
	SimulatedClock simClock;
	ResourceDescriptors resource[NUM_RESOURCES];
} SharedMemory;

void allocateSharedMemory();
void releaseSharedMemory();
void deleteSharedMemory();

void allocateMessageQueues();
void deleteMessageQueues();

SharedMemory* shmemPtr();
int childMsgQptr();
int parentMsgQptr();

#endif
