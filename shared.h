/*
 * Header file for shared memory
 * and message queues
 *
*/

#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>

#define MAX_USER_PROCESS 18
#define MEMORY_SIZE 256

//Structure for Message Queue
typedef struct {
        long mtype;
        int pid;
        int address;
        bool terminate;
        int page;
        int readOrWrite;                //read 1 write 0
} Message;

//Structure for Simulated Clock
typedef struct {
        unsigned int sec;
        unsigned int ns;
} SimulatedClock;

//Structure for page table
typedef struct {
        int pages[32];
        int delimiter;
        int offset;
} PageTable;

typedef struct {
        PageTable ptable;
        int pid;
} ProcessControlBlock;

//Structure for frame table
typedef struct {
        int frames;
        int dirtyBit;
        int referenceBit;
        int read;
        int write;
        int pid;
        int SC;
} FrameTable;

typedef struct {
        SimulatedClock simClock;
        ProcessControlBlock pcb;
        int segFault;
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

