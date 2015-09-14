//Name:  Caleb Priester
//Class: CPSC3220 Sect. 2
//Asg:   Project 2
//Date:  10 March 2015
//
//Description: mythreads.c holds all functionality of the thread library.
//  The library can be initialized and used to create, join, and yield threads
//  and also contains all logic for thread synchronization (locks and condition
//  variables.


#include "mythreads.h"

int interruptsAreDisabled = 0;  //Interrupts are enabled to begin with.

//threadInit is used to initialize the thread library.  It creates the first 
//  node in the linked list (representing main), and initializes the array of 
//  locks for use.
void threadInit() {
   interruptDisable();   //Ensures that this method is uniterrupted

   //Create the first node in the linked list and make it head and tail.
   head = (node_t *)malloc(sizeof(node_t));
   tail = head;

   //Initialize the array of locks and set all locks to unlocked (0).
   locks = (int *)malloc(sizeof(int)*NUM_LOCKS);
   int i;
   for(i = 0; i < NUM_LOCKS; i++) locks[i] = 0;

   //Make the first available id 1 for future created threads.
   availableID = 1;

   //Create the thread representing the main thread, which has id 0 and does
   //  not need a new stack as it already has one.
   head->thread = (thread_t *)malloc(sizeof(thread_t));
   head->thread->id = 0;
   head->thread->stack = NULL;
   head->thread->result = NULL;

   //The thread begins unjoined and unblocked.   
   head->thread->join = -1;
   head->thread->lock = -1;
   head->thread->cond = -1;

   //Next is null as head is the only node in the linked list.
   head->next = NULL;

   interruptEnable();    //Reenables interrupts before returning to user code.

}

//threadCreate is used to create a new thread assigned to the function passed
//  in using the parameters passed in.  This thread is added to the linked list
//  and the context is created.
int threadCreate(thFuncPtr funcPtr, void *argPtr) {

   interruptDisable();   //Ensures this method completes uniterrupted.

   //Keeps track of the thread that called this for context switching.  
   thread_t *curThread = head->thread;

   //Create a new node in the linked list to represent the newly created 
   //  thread.  This thread is put at the front of the linked list to be run 
   //  immediately.
   node_t *new = (node_t *)malloc(sizeof(node_t));
   new->next = head;
   head = new;

   //Actually create the new thread with the next available id.
   new->thread = (thread_t *)malloc(sizeof(thread_t));
   new->thread->id = availableID++;

   //Initialize the new thread's stack and make the thread's context using it.
   new->thread->stack = malloc(STACK_SIZE);
   if(getcontext(&(new->thread->context)) == -1) perror("getcontext!");
   new->thread->context.uc_stack.ss_sp = new->thread->stack;
   new->thread->context.uc_stack.ss_size = STACK_SIZE;

   makecontext(&(new->thread->context), 
      (void (*) (void))wrapFunc, 2, funcPtr, argPtr);

   //The new thread begins with no result.
   new->thread->result = NULL;

   //the new thread is unjoined and unblocked on creation.
   new->thread->join = -1;
   new->thread->lock = -1;
   new->thread->cond = -1;

   //Swap to the new thread's context to be run immediately.
   swapcontext(&(curThread->context), &(head->thread->context));

   interruptEnable();    //Enable interrupts before returning to user code.

   //Return the id of the created thread.
   return new->thread->id;

}

//threadYield is called in order for a thread to give up the processor.
void threadYield() {
   interruptDisable();   //Ensures method completes uniterrupted.

   //Keeps track of current thread for context switching.
   thread_t *curThread = head->thread;

   //Call threadSchedule to move next thread to be run to head of linked list.
   threadSchedule();

   //Actually swap to the next scheduled thread.
   swapcontext(&(curThread->context), &(head->thread->context));
   
   //Free the stack of the thread in doneHead if necessary.
   if(cleanUpNeeded) {
      cleanUp();
      cleanUpNeeded = 0;
   }
   
   interruptEnable();    //Reenable interrupts before returning to user code.
}

//threadSchedule is used to move the next thread to be run to the head of the 
//  linked list.  This thread must niether be joined or blocked.  Interrupts
//  will always be disabled in this method beccause it is only used internally.
void threadSchedule() {

   //If there is only one thread, it will remain the head.
   if(head->next == NULL) return;

   //Move the currently running thread to the back of the line.
   node_t *temp = head;
   head = head->next;
   tail->next = temp;
   tail = tail->next;
   tail->next = NULL;

   //This loop checks the thread at the head node and moves it to the back of
   //  the line if it is joined or blocked.
   while(1) {
      if(head->thread->join == -1) {
         if(head->thread->lock == -1 || locks[head->thread->lock] == 0) {
	    if(head->thread->cond == -1) {
	       if(locks[head->thread->lock] == 0) head->thread->lock = -1;
               break;
            }
	 }
      }
      temp = head;
      head = head->next;
      tail->next = temp;
      tail = tail->next;
      tail->next = NULL;
   }

   return;
}

//threadJoin marks the currently running thread as joined so the scheduler will
//  skip past it if the thread to be joined is running.  Otherwise, the 
//  processor is not given up.  The result passed in is then set to the result
//  of the thread joined to if the result passed in is not NULL.
void threadJoin(int thread_id, void **result) {
   
   interruptDisable();    //Ensures that this method completes uninterrupted.

   //Check to see if the thread to be joined is currently active (in the linked
   //  list).  If it is, the current thread is marked by having its join 
   //  variable set to the thread id passed in and then swaps to the next 
   //  thread to be run.
   node_t *temp = head->next;
   thread_t *curThread = head->thread;
   while(temp != NULL) {
      if(temp->thread->id == thread_id) {
         head->thread->join = thread_id;
	 threadSchedule();
	 swapcontext(&(curThread->context), &(head->thread->context));
	 break;
      }
      temp = temp->next;
   }
   
   //Gets the result of the completed thread joined to.
   if(result != NULL) *result = getResult(thread_id);

   //Frees the stack of the thread in doneHead if necessary.
   if(cleanUpNeeded) {
      cleanUp();
      cleanUpNeeded = 0;
   }

   interruptEnable();    //Reenables interrupts before returning to user code.
   return;
}

//threadExit is always invoked when a thread finished execution.  It moves the 
//  thread from the linked list of active threads 
void threadExit(void *result) {

   interruptDisable();   //Ensures that the method completes uninterrupted.

   //Sets the result of the currently exiting thread to the passed in result.
   head->thread->result = result;

   //If the main thread is invoking this method, exit the program.
   if(head->thread->id == 0) { interruptEnable(); exit(1); }

   //Remove the currently exiting thread from the active linked list.
   node_t *temp = head;
   head = head->next;
   
   //Add the currently exiting thread to the completed linked list (doneHead).
   temp->next = doneHead;
   doneHead = temp;

   //Check all active threads to see if any are joined to the currently exiting
   //  thread and marked them as unjoined (-1) if they are.
   temp = head;
   while(temp != NULL) {
      if(temp->thread->join == doneHead->thread->id)
         temp->thread->join = -1;
      temp = temp->next;
   }

   //Schedule the next active thread for execution.  This does not employ 
   //  threadSchedule, because threadSchedule begins by moving head to the
   //  back of the line.
   while(1) {
      if(head->thread->join == -1) {
         if(head->thread->lock == -1 || locks[head->thread->lock] == 0) {
	    if(head->thread->cond == -1) {
               //The thread is no longer locked if its lock is unlocked AND it
               //  is not blocked on a condition variable.
	       if(locks[head->thread->lock] == 0) head->thread->lock = -1;
               break;
            }
	 }
      }
      if(head->next != NULL) {
         temp = head;
         head = head->next;
         tail->next = temp;
         tail = temp;
         tail->next = NULL;
      }
   }

   //Indicate that the just added doneHead needs to have its stack freed.
   cleanUpNeeded = 1;
   swapcontext(&(doneHead->thread->context), &(head->thread->context));

   return;
}

//threadLock either allows a thread to grab a lock and continue execution, or 
//  marks the thread as waiting for a lock.  When the lock finaly does unlock,
//  the thread will then grab the lock and continue execution.
void threadLock(int lockNum) {
   interruptDisable();   //Ensures that this method completes uninterrupted.

   //Keeps track of currently running thread for context switching.
   thread_t *curThread = head->thread;

   //If the lock is locked, then mark the thread as locked on lockNum and 
   //  switch to the next available thread.
   if(locks[lockNum] == 1) {
      head->thread->lock = lockNum;
      threadSchedule();
      swapcontext(&(curThread->context), &(head->thread->context));
   }

   //If the thread gets here, it locks the lock and continues execution.
   locks[lockNum] = 1;
   interruptEnable();    //Reenables interrupts before returning to user code.
   return;

}

//threadUnlock simply marks the lock at lockNum as unlocked.  No threads are 
//  notified, but the scheduler uses this information to unblock threads.
void threadUnlock(int lockNum) {
   
   interruptDisable();   //Ensures that this method completes uninterrupted.
   
   //Unlocks the lock.
   if(locks[lockNum] == 1) locks[lockNum] = 0;

   interruptEnable();    //Reenables interrupts before returning to user code.

   return;
}


//threadWait unlocks a lock currently held by the calling thread and blocks the
//  thread on a condition variable until signalled.  When signalled, the thread
//  reacquires the lock and continues execution.
void threadWait(int lockNum, int conditionNum) {

   interruptDisable();   //Ensures that this method completes uninterrupted.

   //Keeps track of the currently running thread for context switching.
   thread_t *curThread = head->thread;

   //It is an error if this method indicates a lock already unlocked.
   if (locks[lockNum] == 0) {
      printf("ERROR:\t Wait called on unlocked lock.\n");
      exit(0);
   }

   //The lock at lockNum is unlocked and the thread is marked as locked on 
   //  lockNum and blocked on conditionNum.  The next thread is then scheduled
   //  and switched to.  Even when signalled, the current thread will still 
   //  need to wait on the lock it used to hold.
   locks[lockNum] = 0;
   head->thread->lock = lockNum;
   head->thread->cond = conditionNum;
   threadSchedule();
   swapcontext(&(curThread->context), &(head->thread->context));

   //Relock the lock when scheduled and continue execution.
   locks[lockNum] = 1;

   interruptEnable();    //Reenables interrupts before returning to user code.
   
}

//threadSignal unblocks ONE thread that is blocked on a condition at a lock.
void threadSignal(int lockNum, int conditionNum) {
   
   interruptDisable();   //Ensures that this method completes uninterrupted.

   //Searches for first thread in list blocked on a conditition at a lock and 
   //  then unblocks that thread.
   node_t *temp = head;
   while(temp != NULL) {
      if(temp->thread->lock == lockNum && temp->thread->cond == conditionNum) {
	 temp->thread->cond = -1;
	 break;
      }
      temp = temp->next;
   }

   interruptEnable();    //Reenables interrupts before returning to user code.
}

//wrapFunction is used to know when a thread completes its specified function. 
//  It then invokes threadExit using the result returned by the function.
void wrapFunc(thFuncPtr funcPtr, void *argPtr) {
   
   interruptEnable();   //Ensures that interrupts are enabled even when thread
                        //  is running.

   //Call the specified function and store the result.
   void *result = funcPtr(argPtr);

   //Exit the thread upon completion and pass its result for storage.
   threadExit(result);
   return;
}

//getResult is used in order to find the result of a completed thread.
void *getResult(int thread_id) {

   //Travers the done linked list looking for specified thread id.  If found, 
   //  return its result.
   node_t *temp = doneHead;
   while(temp != NULL) {
     if(temp->thread->id == thread_id) return temp->thread->result;
     temp = temp->next;
   }

   //If not found, return NULL.
   return NULL;
}


//interruptDisable is used to disallow interrupts.
static void interruptDisable() {
   assert(!interruptsAreDisabled);
   interruptsAreDisabled = 1;
}

//interruptsEnable is used to allow interrupts.
static void interruptEnable() {
   assert(interruptsAreDisabled);
   interruptsAreDisabled = 0;
}

//cleanUp frees the stack of the most recently completed thread 
//  (thread at doneHead).
void cleanUp() {
   free(doneHead->thread->stack);
}

//finalCleanUp is the destructor of the library.  It frees all threads active 
//  and completed that are not main.
void finalCleanUp(void) {

   interruptDisable();  //Ensures that this method completes uninterrupted.

   free(locks);

   //Clean up all active threads except main's stack.
   node_t *temp;
   while(head != NULL) {
      temp = head;
      if(head->thread->id) free(head->thread->stack);
      free(head->thread->result);
      free(head->thread);
      head = head->next;
      free(temp);
   }

   //Clean up all inactive threads.
   while(doneHead != NULL) {
      temp = doneHead;
      free(doneHead->thread->result);
      free(doneHead->thread);
      doneHead = doneHead->next;
      free(temp);
   }


}

//printThreadIDs is only used for debugging purposes.
void printThreadIDs() {
   interruptDisable();

   node_t *temp = head;
   while(temp != NULL) {
      printf("%d ", temp->thread->id);
      temp = temp->next;
   }
   printf("\n\n");

   interruptEnable();
}
