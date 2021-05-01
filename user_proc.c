#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/msg.h>

#include "shared.h"

//variables for shmem and msg q
SharedMemory * shared = NULL;
Message msg;

//message queue ids
static int pMsgQID;
static int cMsgQID;

void allocation();

int main(int argc, char * argv[]) {



	printf("user: hello world\n");


	return EXIT_SUCCESS;

}
