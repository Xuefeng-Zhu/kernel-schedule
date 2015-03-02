#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int fibonacci(int n)
{
   if (n <= 0)
      return 0;
   else if (n == 1)
      return 1;
   else
      return fibonacci(n-1) + fibonacci(n-2);
}

/* Write to /proc file to register the process with RMS */
void register_process(unsigned long pid, unsigned long period, unsigned long computation) {
	printf("%lu: registering...\n", pid);
	char echo_buf[100];
	sprintf(echo_buf, "echo R %lu %lu %lu > /proc/mp2/status", pid, period, computation);
	system(echo_buf);
	printf("%lu: finish registering...\n", pid);
}

/* Helper function to check whether a program is successfully registerd in /proc file.
   return 1 is successful, 0 otherwise.
*/
int is_registerd(unsigned long pid) {
	printf("%lu: checking...\n", pid);

 	int is_registerd = 0;
 	FILE *fp; // define file pointer
 	fp = fopen("/proc/mp2/status", "r");
 	if(fp == NULL) {
 		printf("Unable to open /proc/mp2/status for reading\n");
 		return 0;
 	}

 	unsigned long fpid; // this will hold the current PID from file
 	while(fscanf(fp, "PID: %lu", &fpid) != EOF) {
 		printf("%lu\n", fpid);
 		if(fpid == pid) {
 			is_registerd = 1;
 			break;
 		}
 	}
 	fclose(fp);
 	printf("%lu: finish checking...\n", pid);
 	return is_registerd;
 }

/* Write to /proc file to signal that we have finished task for this period and yield. */
 void yield(unsigned long pid) {
 	printf("%lu: yielding...\n", pid);
 	char echo_buf[100];
 	sprintf(echo_buf, "echo Y %lu > /proc/mp2/status", pid);
 	system(echo_buf);
 	printf("%lu: finish yielding...\n", pid);
 }

 /* Write to /proc file to signal that we have finished the application and de-register the process */
void de_register(unsigned long pid) {
	printf("%lu: deregistering...\n", pid);
	char echo_buf[100];
	sprintf(echo_buf, "echo D %lu > /proc/mp2/status", pid);
	system(echo_buf);
}

unsigned long compute_time(){
	struct timeval start_time, end_time;
	unsigned long time_elapsed;

	gettimeofday(&start_time);
	fibonacci(30);
 	gettimeofday(&end_time);
 	time_elapsed = (end_time.tv_sec - start_time.tv_sec)*1000 + (end_time.tv_usec - start_time.tv_usec)/1000;
 	return time_elapsed;

}

/* Main function that takes two arguments from users: period and number of jobs. */
int main(int argc, char *argv[]) {
	unsigned long pid = getpid();
	unsigned long period = atoi(argv[1]);
	int iterations  = atoi(argv[2]);
	unsigned long computation = compute_time();
    printf("%lu\n", computation);

	register_process(pid, period, computation);
	if(!is_registerd(pid)) {
		printf("%lu: unable to register. \n", pid);
		exit(1);
	}
	printf("%lu: registerd\n", pid);
	yield(pid);

	while(iterations >0) {
		fibonacci(30);
		yield(pid);
		iterations--;
	}
	de_register(pid);
	return 0;

}



