#ifndef GTTHREAD_H
#define GTTHREAD_H

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>
#include "steque.h"
#include <ucontext.h>

/* Define gtthread_t and gtthread_mutex_t types here */
/* Thread state enum
*/
typedef enum threadstate_t {
	RDY,
	BLOCK, //Blocked by a mutex
	CANCELREQ, //Cancel request for the thread
	TERMIN // Thread is completed

 } threadstate_t;
 /* Define the gthread struct separately from the gtthread_t (thread ID)
 * Because the thread ID is returned by the functions and we dont want
 * the thread struct to be returned  (and the mutex struct also uses the
 * thread ID */
typedef long int gtthread_t; //this is the thread ID, to mirror pthread_t

/* Data structure for a single thread */
typedef struct gtthread {

	gtthread_t thread_id; //thread ID
	ucontext_t context; //context 
	threadstate_t state;// thread state 
	steque_t* joinlist; //queue for threads waiting to join this one
	void* returnval; // return value

} gtthread;

/* Mutex struct */
/* Here we dont define two types since the mutex is never returned by any
of the API functions */
typedef struct gtthread_mutex_t{
	gtthread_t lock_thread_ID; // ID of thread holding the lock currently
	steque_t waiting_list; //list of threads waiting on mutex 

}  ;


void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);
int  gtthread_join(gtthread_t thread, void **status);
void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);


int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);
int  gtthread_mutex_destroy(gtthread_mutex_t *mutex);
#endif
