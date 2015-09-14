//threadtest.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mythreads.h"

int num_threads = 100;

void *t1 (void *arg)
{
	int param = *((int*)arg);
	printf("t1 started %d\n",param);

	//if(param != 2) {threadJoin(2, NULL);
	//printf("joined #2\n");}
	threadYield();
if(param%3 == 0) threadLock(0);
else if(param%3 == 1) threadLock(1);
else {threadLock(2); if(param%5) threadWait(2, 0); else threadWait(2, 1);}
	int* result = malloc(sizeof(int));
	*result = param + 1;
	printf ("added 1! (%d)\n",*result);
	threadYield();
	threadYield();
	threadYield();
	threadYield();
	threadYield();
	threadYield();
	threadYield();
	threadYield();
	threadYield();
	threadYield();
if(param%3 == 0) threadUnlock(0);
threadYield();
if(param%3 == 1) threadUnlock(1);
threadYield();
if(param%3 == 2) threadUnlock(2);
threadSignal(2, 1);

	printf("t1: done result=%d\n",*result);
threadYield();
threadSignal(2, 0);
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
	for(i = 0; i < num_threads; i++) {
	   p[i] = rand()%100;
           //p[i] = 0;
	   ids[i] = threadCreate(t1, (void*)&(p[i]));
	   printf("Created thread %d.\n", ids[i]);
	}
	for(i = 0; i < num_threads; i++) {
	   threadJoin(ids[i], (void*)&(results[i]));
	   printf("joined #%d --> %d.\n",ids[i], *(results[i]));
	}

}

