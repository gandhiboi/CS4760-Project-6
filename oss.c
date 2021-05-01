/*
	Kenan Krijestorac
	Project 6 - Resource Management
	Due: 26 April 2021
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

#include "shared.h"
#include "queue.h"

int main(int argc, char* argv[]) {

	printf("hello world!\n");


	execl("./user_proc", "user_proc", (char*)NULL);

	return EXIT_SUCCESS;


}
