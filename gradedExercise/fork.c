#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// if fork == 0 : inside child
// if fork != 0 : Inside parent

int main() {
	// Inside Main
	printf("\n\n------Start of program----\n\n");
	fflush(stdout);

	if (fork() != 0) {
		// Inside Parent 1
		if (fork() != 0) {
			// Inside Parent 2
			printf("Inside Parent2, hello1\n");
			fflush(stdout);
		} else {
			// Inside Parent1, Child1
			fork();
		}
		printf("Inside Parent 1, hello2\n");
		fflush(stdout);
	} else {
		// Inside Child 1
		if (fork() != 0) {
			// Inside Child 1, Parent 1
			printf("Inside Child 1 Parent 1, hello3\n");
			fflush(stdout);
		} else {
			// Inside Child 2
			fork();
		}
		// Inside Child 1
		printf("Inside Child 1, hello4\n");
		fflush(stdout);
	}

	// Inside Main
	printf("Inside Main, Final. hello5\n");
	fflush(stdout);
	return 0;
}
