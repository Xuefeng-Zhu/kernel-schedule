#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


unsigned long factorial(int n);

/* Write to /proc file to register the process with RMS */
void register_process(unsigned long pid, unsigned long period, unsigned long computation) {
	printf("%lu: registering...\n", pid);
	char echo_buf[100];
	sprintf(echo_buf, "echo R %lu %lu %lu > /proc/mp2/status", pid, period, computation);
	system(echo_buf);
}

/* Helper function to check whether a program is successfully registerd in /proc file.
   return 1 is successful, 0 otherwise.
*/
int is_registerd(unsigned long pid) {
 	int is_registerd = 0;
 	FILE *fp; // define file pointer
 	fp = fopen("/proc/mp2/status", "r");
 	if(fp == NULL) {
 		printf("Unable to open /proc/mp2/status for reading\n");
 		return 0;
 	}

 	unsigned long fpid; // this will hold the current PID from file
 	while(fscanf(fp, "%lu", &fpid) != EOF) {
 		if(fpid == pid) {
 			is_registerd = 1;
 			break;
 		}
 	}
 	fclose(fp);
 	return is_registerd;
 }

/* Write to /proc file to signal that we have finished task for this period and yield. */
 void yield(unsigned long pid) {
 	printf("%lu: yileding...\n", pid);
 	char echo_buf[100];
 	sprintf(echo_buf, "echo Y %lu > /proc/mp2/status", pid);
 	system(echo_buf);
 }

 /* Write to /proc file to signal that we have finished the application and de-register the process */
void de_register(unsigned long pid) {
	printf("%lu: deregistering...\n", pid);
	char echo_buf[100];
	sprintf(echo_buf, "echo D %lu > /proc/mp2/status", pid);
	system(echo_buf);
}

unsigned long compute_time(){
	struct timeval time;
	unsigned long time_elapsed;
	
	gettimeofday(&time);                                                    
	unsigned int start_time = time.tv_usec;
	factorial(10);
 	gettimeofday(&time);
 	time_elapsed = time.tv_usec - start_time;
 	return time_elapsed;
	
}

/* Main function that takes two arguments from users: period and number of jobs. */
int main(int argc, char *argv[]) {
	unsigned long pid = getpid();
	unsigned long period = atoi(argv[2]);
	int iterations  = atoi(argv[3]);
	unsigned long computation = compute_time();
	
	register_process(pid, period, computation);
	if(!is_registerd(pid)) {
		printf("%lu: unable to register. \n", pid);
		exit(1);
	}
	printf("%lu: registerd\n", pid);
	yield(pid);
	
	while(iterations >0) {
		factorial(10);
		yield(pid);
		iterations--;
	}
	de_register(pid);
	return 0;

}

unsigned long factorial(int n) {
	if(n == 1)
		return 1;
	else
		return n * factorial(n-1);
}


