#include <stdio.h>
#include <stdlib.h>
#include "gtthread.h"

/* Tests creation.
   Should print "Hello World!" */

void *thr1(void *in) {
  printf("Here in function thr1 start, called from thread %i!\n",(int) in);
  fflush(stdout);
  for(int i = 0; i < 50; i++)
  {
  	printf("Thread %i function thr1 \n",(int) in);
  }
  printf("Leaving thr1 with thread %i",(int) in);
  return (void *) gtthread_self();
}

int main() {
  gtthread_t t1;
  gtthread_t t2;
  void* ret1;
  void* ret2;
  gtthread_init(200);
  gtthread_create( &t1, thr1, (void *) 1);
  gtthread_create( &t2, thr1, (void *) 2 );
  gtthread_join(t1, &ret1);
  printf("ret from thread 1 after joining is %i \n",(int) ret1);
  if(gtthread_equal(t2,(gtthread_t) ret1))
      printf("Error in gtthread equals method \n");
  if(!gtthread_equal(t1,(gtthread_t) ret1))
      printf("Error in gtthread equals method \n");
  gtthread_join(t2, &ret2);
  if(gtthread_equal(t2,(gtthread_t) ret2))
      printf("Equals works for thread 2 \n");


 // gtthread_yield();

  return EXIT_SUCCESS;
}
