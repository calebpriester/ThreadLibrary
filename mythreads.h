//Name:  Caleb Priester
//Class: CPSC3220 Sect. 2
//Asg:   Project 2
//Date:  10 March 2015
//
//Description: mythreads.h contains method headers for all methods used by 
//  mythreads.c.  It also contains the definitons for the data structures
//  used to keep track of threads and the linked list nodes.

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define STACK_SIZE (16*1024)  
#define NUM_LOCKS 10 
#define CONDITIONS_PER_LOCK 10 

void __attribute__ ((destructor)) finalCleanUp(void);


//This is the structure used to keep track of threads.
typedef struct thread {

   int id;                     //Thread ID
   
   ucontext_t context;         //Thread Context

   char *stack;                //Stack used for thread

   void *result;               //Result of the thread's function

   int join;                   //ID of thread joined to (-1 if not joined)
   int lock;                   //Lock thread is waiting on (-1 if not waiting)
   int cond;                   //Cond thread is waiting on (-1 if not waiting)

} thread_t;

//This is the structure used to keep track of the nodes in the linked list.
typedef struct node {
   
   thread_t *thread;           //Thread stored at node
   struct node *next;          //Next node in list

} node_t;

node_t *head;                  //First node in linked list
node_t *tail;                  //Last node in linked list
node_t *doneHead;              //First node in linked list 
                               //  holding completed threads
int *locks;                    //Array of locks

int availableID;               //Next available thread id to be assigned
int cleanUpNeeded;             //Indicates that the stack of the thread held
                               //  in doneHead needs to be freed

//the type of function used to run your threads
typedef void *(*thFuncPtr) (void *);

extern void threadInit();
extern int threadCreate(thFuncPtr funcPtr, void *argPtr);
extern void threadYield(); 
extern void threadSchedule();
extern void threadJoin(int thread_id, void **result);

//exits the current thread -- closing the main thread, will terminate the program
extern void threadExit(void *result); 

extern void threadLock(int lockNum); 
extern void threadUnlock(int lockNum); 
extern void threadWait(int lockNum, int conditionNum); 
extern void threadSignal(int lockNum, int conditionNum);

extern void wrapFunc(thFuncPtr funcPtr, void *argPtr);
extern void *getResult(int thread_id);
extern void cleanUp();

//this 
extern int interruptsAreDisabled;
static void interruptDisable();
static void interruptEnable();

extern void printThreadIDs();
