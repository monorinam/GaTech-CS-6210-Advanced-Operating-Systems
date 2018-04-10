/**********************************************************************
gtthread_sched.c.  

This file contains the implementation of the scheduling subset of the
gtthreads library.  A simple round-robin queue should be used.
 **********************************************************************/
/*
  Include as needed
*/

#include "gtthread.h"
#include "steque.h"
#define FAILURE -1
#define SUCCESS 0
#define RUNQ 1
#define DEADQ 2
#define TNOTFND 1


/* 
   Students should define global variables and helper functions as
   they see fit.
 */
sigset_t vtalrm;
struct itimerval *T;
struct sigaction act;
/* List of runnable threads */
steque_t *run_queue;
/* List of threads not currently running (blocked,finished,cancelled) */
steque_t *dead_list;
// Is the thread already initialized? :flag
int init_done = 0;
//Count of all threads started
long int thread_cnt = 0;
long quantum;
/*Finds a thread using thread ID in one of the two thread queues 
Returns the queue number in the passed queue parameter, and the thread
*/
static gtthread* find_thread_in_queue(gtthread_t thread, int *queue){
  //Default value if the thread is not found in either queue
  *queue = 0;
  gtthread *temp_thread;
  gtthread *found_thread;
  int run_length = steque_size(run_queue);
  int dead_length = steque_size(dead_list);
  // Look in run queue first
  for(int i = 0; i < run_length; i++){
    temp_thread = steque_front(run_queue);
    if(temp_thread->thread_id == thread)
    {
      //we care about the order of the run queue
      // So cycle through the whole thing anyway
      found_thread = steque_front(run_queue);
      *queue = RUNQ; 
      
    }
    steque_cycle(run_queue);

  }

  if(*queue)
    return found_thread;
  //Else look in the terminate queue
  for(int i = 0; i < dead_length; i++){
    temp_thread = steque_front(dead_list);
    if(temp_thread->thread_id == thread)
    {
      // We dont care about the order of the dead queue,
      // so return straight away
      *queue = DEADQ;
      return temp_thread;

    }
    steque_cycle(dead_list);
  }

  //Did not find thread
  return NULL;

}
/* This clears the join queue of the current thread
that is cancelled or completed and adds it to the 
end of the run queue
*/
static void clear_join_queue(gtthread* thread){
  while(!steque_isempty(thread->joinlist)){
    steque_enqueue(run_queue,(gtthread *)steque_pop(thread->joinlist));
  }
}
static void cancel_thread(gtthread *this_thread){
  // Mark the thread as done
  // Then clear its join queue and add to run queue
  // Add the thread to the dead queue
  this_thread->state = TERMIN;
  clear_join_queue(this_thread);
  steque_enqueue(dead_list,this_thread);
}

static void run_next_thread(gtthread *curr_thread){
  gtthread *next_thread;
  do{
      next_thread = (gtthread*) steque_front(run_queue);
      if(next_thread->state == CANCELREQ){
        cancel_thread(next_thread);
        steque_pop(run_queue);
      }
    }while(next_thread->state == CANCELREQ);
  swapcontext(&curr_thread->context,&next_thread->context);
}
/*
* These two function are not an API function
* They only public since they are called
* by the mutex functions.
*/
/*
1.
Remove the thread from the run queue of the current thread
* And run the next thread 
* This thread is added to the wait queue of the mutex
* and runs when the mutex unlocks this thread
*/
void block_thread()
{
  //Pop the current thread from the run queue
  gtthread *curr_thread = (gtthread*) steque_pop(run_queue);
  //Mark thread as blocked and add to dead_list
  curr_thread->state = BLOCK;
  steque_enqueue(dead_list,curr_thread);
  //Run the next thread
  run_next_thread(curr_thread);

}
/* 2.
Remove a blocked thread from the dead_list
and add it to the running queue
*/
void unblock_thread(gtthread_t ID)
{
  //Find the thread using the thread ID in the dead list
  int which_queue;
  gtthread *thread;
  thread = find_thread_in_queue(ID,&which_queue);
  if(which_queue == DEADQ && thread->state == BLOCK){
    //Should always be the case
    //Add the thread to the running queue
    thread->state = RDY;
    //The found thread is always at the start of the dead list
    // after find_thread_in_queue is called
    steque_pop(dead_list);
    steque_enqueue(run_queue,thread);
  }

}
/* @Citation: From defcon.c
*/
// This handler puts the old thread at the end of the steque
// and starts the next scheduled thread in the queue
void alrm_handler(int sig){
  gtthread *curr_thread;

  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  //Add old thread to end of run queue
  steque_enqueue(run_queue, curr_thread = (gtthread*) steque_pop(run_queue));
  //Run next thread
  run_next_thread(curr_thread);
  // Unblock signals
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);

}
static void start_routine_wrapper(void *(*start_routine)(void *), void *arg)
{
    void * retval = start_routine(arg);
    /* Mark current thread as done */
    gtthread * curr_thread = (gtthread *) steque_front(run_queue);
    curr_thread->returnval = retval;
    curr_thread->state = TERMIN;
    steque_enqueue(dead_list, curr_thread);
}


/*
  The gtthread_init() function does not have a corresponding pthread equivalent.
  It must be called from the main thread before any other GTThreads
  functions are called. It allows the caller to specify the scheduling
  period (quantum in micro second), and may also perform any other
  necessary initialization.  If period is zero, then thread switching should
  occur only on calls to gtthread_yield().

  Recall that the initial thread of the program (i.e. the one running
  main() ) is a thread like any other. It should have a
  gtthread_t that clients can retrieve by calling gtthread_self()
  from the initial thread, and they should be able to specify it as an
  argument to other GTThreads functions. The only difference in the
  initial thread is how it behaves when it executes a return
  instruction. You can find details on this difference in the man page
  for pthread_create.
 */
void gtthread_init(long period){
  if(init_done)
  {
    perror("Thread already initialized, terminating init..");
    exit(EXIT_FAILURE);
  }
  quantum = period;
  // Set up the timer
  if(quantum != 0)
  {
    //Added from defcon.c example
    sigemptyset(&vtalrm);
    sigaddset(&vtalrm,SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);

    /* 
    Setting up the alarm
    */
    T = (struct itimerval*) malloc(sizeof(struct itimerval));
    T->it_value.tv_sec = T->it_interval.tv_sec = 0;
    T->it_value.tv_usec = T->it_interval.tv_usec = quantum;

    setitimer(ITIMER_VIRTUAL, T, NULL);

    /*
    Setting up the handler
    */
    memset (&act, '\0', sizeof(act));
    act.sa_handler = &alrm_handler;
    if (sigaction(SIGVTALRM, &act, NULL) < 0) {
      perror ("sigaction");
      return;
    }

  }
  // Define main thread
  gtthread *main_thread = (gtthread*) malloc(sizeof(gtthread));

  if(main_thread == NULL){
    perror("Main thread not initialized");
    exit(EXIT_FAILURE);
  }
 
  // Set up the main thread
  main_thread->thread_id = ++thread_cnt;

  //Set up thread context
  //@Citation: Adapted from: https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/rmctxt.htm
  if(getcontext(&main_thread->context) == -1)
  {
    perror("Error in context");
    exit(EXIT_FAILURE);
  }
  main_thread->context.uc_stack.ss_sp = (char *) malloc(SIGSTKSZ);
  if(main_thread->context.uc_stack.ss_sp == NULL)
  {
    perror("Not enough storage for stack");
    exit(EXIT_FAILURE);
  }
  main_thread->context.uc_stack.ss_size = SIGSTKSZ;
  main_thread->joinlist =(steque_t*) malloc(sizeof(steque_t));
  steque_init(main_thread->joinlist);
  //Main thread state
  main_thread->state = RDY;

  //Initialize queues
  run_queue = (steque_t *) malloc (sizeof(steque_t));
  dead_list = (steque_t *) malloc (sizeof(steque_t));

  steque_init(run_queue);
  steque_init(dead_list);

  //Add mainthread to the thread queue
  steque_enqueue(run_queue, main_thread);


  init_done = 1;

}


/*
  The gtthread_create() function mirrors the pthread_create() function,
  only default attributes are always assumed.
 */
int gtthread_create(gtthread_t *thread,
		    void *(*start_routine)(void *),
		    void *arg){
  if(!init_done)
  {
    perror("Error: Threads not initialized");
    return FAILURE;
  }
  /*Create a new instance of thread, add context and then 
  add thread to the run queue */
  gtthread *this_thread;
  gtthread *curr_thread;
  sigprocmask(SIG_BLOCK, &vtalrm, NULL);
  if((this_thread = (gtthread *) malloc(sizeof(gtthread))) == NULL)
  {
    perror("Error: Cannot allocate memory for new thread");
    //Unblock thread
    sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
    //exit(EXIT_FAILURE);
    return FAILURE;
  }
  //Create the join list
  this_thread->joinlist = (steque_t*) malloc(sizeof(steque_t));
  // Add thread characteristics
  this_thread->thread_id = ++ thread_cnt;
  *thread = thread_cnt;
  this_thread->state = RDY;
  steque_init(this_thread->joinlist);

  /* Context for this new thread */
  if(getcontext(&this_thread->context) == -1)
  {
    perror("Error in context");
    return FAILURE;
  }
  this_thread->context.uc_stack.ss_sp = (char *) malloc(SIGSTKSZ);
  if(this_thread->context.uc_stack.ss_sp == NULL)
  {
    perror("Not enough storage for stack");
    return FAILURE;
  }
  this_thread->context.uc_stack.ss_size = SIGSTKSZ;
  /*Get the caller */
  curr_thread = (gtthread* ) steque_front(run_queue);
  this_thread->context.uc_link = &curr_thread->context;
  

  makecontext(&this_thread->context, (void (*)(void)) start_routine_wrapper, 2, start_routine, arg);
  /* Add this thread to queue */
  steque_enqueue(run_queue,this_thread);
  sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
  return SUCCESS;
  
}

/*
  The gtthread_join() function is analogous to pthread_join.
  All gtthreads are joinable.
 */

int gtthread_join(gtthread_t thread, void **status){
  int which_queue;
  gtthread *target,*curr;

  // Block alarms
  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  //Find the target thread
  target = find_thread_in_queue(thread,&which_queue);
  if(target == NULL){
    sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);
    return TNOTFND; //thread not found
  }
  //If the thread isn't completed (not in terminate queue)
  // If the thread has a cancel request, wait til it is to be run next
  // and then cancel it in the run_next_thread method.
  // Here we just join to it anyway
  if(which_queue == RUNQ)
  {
      curr=(gtthread *) steque_pop(run_queue);
      //Add current thread to the join queue of the target thread
      steque_enqueue(target->joinlist,curr);
      //Now run the next thread from the run queue
      run_next_thread(curr);
    
  }
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);
  //Set status
  if(status)
    *status = target->returnval;

  return 0; //success
}

/*
  The gtthread_exit() function is analogous to pthread_exit.
 */
void gtthread_exit(void* retval){
  gtthread *curr_thread;
  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  /*
  Take the thread from the run queue
  Mark its status as TERMIN
  Add the thread to the dead list
  Reschedule all the threads on the join list of this thread 
  So they are not lost from the run queue
  Then change context to the next thread in the run queue
  */
  curr_thread = (gtthread*) steque_pop(run_queue);
  clear_join_queue(curr_thread);
  curr_thread->returnval = retval;
  curr_thread->state = TERMIN;
  steque_enqueue(dead_list,curr_thread);
  run_next_thread(curr_thread);
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);


}


/*
  The gtthread_yield() function is analogous to pthread_yield, causing
  the calling thread to relinquish the cpu and place itself at the
  back of the schedule queue.
 */
void gtthread_yield(void){
  gtthread *curr_thread;

  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  /* The current thread is moved to the end of the run queue
  * And the context is swapped to the next thread 
  */
  curr_thread = (gtthread *)steque_pop(run_queue);
  steque_enqueue(run_queue,curr_thread);
  run_next_thread(curr_thread);

  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);


}

/*
  The gtthread_yield() function is analogous to pthread_equal,
  returning zero if the threads are the same and non-zero otherwise.
 */
int  gtthread_equal(gtthread_t t1, gtthread_t t2){
  return (t1 == t2) ? 1 : 0;

}

/*
  The gtthread_cancel() function is analogous to pthread_cancel,
  allowing one thread to terminate another asynchronously.
 */

int  gtthread_cancel(gtthread_t thread){
  /* Here mark the thread status as CANCELREQ 
  *  and the thread is transferred to the terminate queue
  *  the next time it is to be run. This is similar to how
  *  pthread_cancel works
  */
  gtthread *cancelling_thread;
  int which_queue;


  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  cancelling_thread = find_thread_in_queue(thread,&which_queue);
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);

  if(cancelling_thread == NULL)
    return TNOTFND; //not found error
  if(which_queue == RUNQ){
    // found in run queue, need to mark as cancelled
    cancelling_thread->state = CANCELREQ;
  }
  
  //otherwise thread is already terminated, so no need to mark as cancelled
  return 0;
}

/*
  Returns calling thread.
 */
gtthread_t gtthread_self(void){
  gtthread_t ID;
  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  ID = ((gtthread*)steque_front(run_queue))->thread_id;
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);
  return ID;

}
