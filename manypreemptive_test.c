#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#include <string.h>
#include <unistd.h>
#include "mythreads.h"

#define BIG 1000
#define TIMER_INTERVAL_US 10

int num_threads = 100;

//this function will be called
//everytime that the timer fires
//...then I call yield.
void timer_handler (int signum) 
{
	if (!interruptsAreDisabled)
	{
		//printf("--------->$$$$ interrupting\n");
		//printThreadIDs();
                threadYield();
	}
} 

//setup the timer that will preempt your thread
//library
void set_timer()
{
	struct sigaction sa; 
	struct itimerval timer; 

	/* Install timer_handler as the signal handler for SIGVTALRM.  */ 
	memset (&sa, 0, sizeof (sa)); 
	sa.sa_handler = &timer_handler; 
	sigaction (SIGALRM, &sa, NULL); 

  	/* Configure the timer to expire after TIMER_INTERVAL_US usec...  */ 
	timer.it_value.tv_sec = 0; 
	timer.it_value.tv_usec = TIMER_INTERVAL_US; 

	/* ... and every TIMER_INTERVAL_US usec after that.  */ 
	timer.it_interval.tv_sec = 0; 
	timer.it_interval.tv_usec = TIMER_INTERVAL_US; 

  	/* Start a virtual timer. It counts down whenever this process is executing.  */ 
	if (0 != setitimer (ITIMER_REAL, &timer, NULL))
	{
		perror("setitimer error");
	} 
}

void *t1 (void *arg)
{
	int param = *((int*)arg);
//	printf("t1 started %d\n",param);

	int* result = malloc(sizeof(int));
	
	*result = param;
        int i;
	//if(param%2) threadLock(1);
        //else threadLock(0);
	for (i=0; i < (param * BIG); i++)
	{
     	   *result += 1;
	}

	
//	printf ("finished with math! (%d)\n",*result);
	
	sleep(1);
//	printf("t1: done result=%d\n",*result);
        //if(param%2) threadUnlock(1);
        //else threadUnlock(0);
	return result;
}



int main(void) {

        srand(time(NULL));
	int *results[num_threads];
        int p[num_threads];
	int ids[num_threads];
	int i;
	//initialize the threading library. DON'T call this more than once!!!
	threadInit();
        set_timer();

	for(i = 0; i < num_threads; i++) {
	   //p[i] = rand()%50;
           p[i] = 0;
	   ids[i] = threadCreate(t1, (void*)&(p[i]));
//	   printf("Created thread %d.\n", ids[i]);
	}
	for(i = 0; i < num_threads; i++) {
	   threadJoin(ids[i], (void*)&(results[i]));
//	   printf("joined #%d --> %d.\n",ids[i], *(results[i]));
	}

}
