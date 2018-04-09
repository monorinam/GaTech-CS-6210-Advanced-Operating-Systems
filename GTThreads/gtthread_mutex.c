/**********************************************************************
gtthread_mutex.c.  

This file contains the implementation of the mutex subset of the
gtthreads library.  The locks can be implemented with a simple queue.
 **********************************************************************/

/*
  Include as needed
*/


#include "gtthread.h"
#define DEFID -1 //default ID for lock.
sigset_t vtalrm;
/*
  The gtthread_mutex_init() function is analogous to
  pthread_mutex_init with the default parameters enforced.
  There is no need to create a static initializer analogous to
  PTHREAD_MUTEX_INITIALIZER.
 */
int gtthread_mutex_init(gtthread_mutex_t* mutex){
  if(mutex == NULL)
    return 1;
  mutex->lock_thread_ID = DEFID;
  mutex->waiting_list = (steque_t *) malloc(sizeof(steque_t));
  sigemptyset(&vtalrm);
  sigaddset(&vtalrm, SIGVTALRM);
  return 0;

}

/*
  The gtthread_mutex_lock() is analogous to pthread_mutex_lock.
  Returns zero on success.
 */
/* If the mutex lock has not been acquired, the calling thread
* can acquire the lock, and start executing. If the lock has
* been acquired, the calling thread has to wait in the waiting
* queue of the mutex and the next thread starts executing until
* the lock is released for this thread
*/
int gtthread_mutex_lock(gtthread_mutex_t* mutex){
  gtthread_t this_thread;

  if(mutex == NULL){
    perror("Error: Mutex Does Not Exist \n");
    return 1;
  }

  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  this_thread = gtthread_self();
  if(mutex->lock_thread_ID == DEFID)
  {
    mutex->lock_thread_ID = this_thread;
  }
  else{
  /*This thread needs to be in the waiting queue
  of mutex and next thread should execute */
  steque_enqueue(mutex->waiting_list,(void *) this_thread);
  //Give over CPU to next runnable thread
  block_thread();
  }
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);
  return 0;
}
`
/*
  The gtthread_mutex_unlock() is analogous to pthread_mutex_unlock.
  Returns zero on success.
  If the lock is acquired already, then unblock the threads in the 
  waiting queue of the mutex and release the lock to the next thread
 */
int gtthread_mutex_unlock(gtthread_mutex_t *mutex){
  if(mutex == NULL){
    perror("Error: Mutex Does Not Exist \n");
    return 1;
  }
  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  //Mutex is locked
  if(mutex->lock_thread_ID != DEFID){
    mutex->lock_thread_ID = DEFID; //Should the next thread get the lock
    if(!steque_isempty(mutex->waiting_list)){
      //If there is a waiting thread add it to list
      // Of runnable threads
      unblock_thread(steque_pop(mutex->waiting_list));
    }

  }
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);
}

/*
  The gtthread_mutex_destroy() function is analogous to
  pthread_mutex_destroy and frees any resourcs associated with the mutex.
*/
int gtthread_mutex_destroy(gtthread_mutex_t *mutex){
  sigprocmask(SIG_BLOCK,&vtalrm,NULL);
  mutex->lock_thread_ID = DEFID;
  steque_destroy(mutex->waiting_list);
  free(mutex->waiting_list);
  sigprocmask(SIG_UNBLOCK,&vtalrm,NULL);

}
