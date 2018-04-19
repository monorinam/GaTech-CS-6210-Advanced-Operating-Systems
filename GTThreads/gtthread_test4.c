#include <stdio.h>
#include <stdlib.h>
#include "gtthread.h"

/* Tests creation.
   Should print "Hello World!" */
gtthread_mutex_t test_mutex;
int count = 0;
void *thr1(void *in) {
  gtthread_mutex_lock(&test_mutex);
  count++;
  gtthread_mutex_unlock(&test_mutex);
    /**
  printf("Here in function thr1 start, called from thread %i!\n",(int) in);
  fflush(stdout);
  for(int i = 0; i < 50; i++)
  {
  	printf("Thread %i function thr1 \n",(int) in);
  }
  printf("Leaving thr1 with thread %i",(int) in);
  return (void *) gtthread_self();
  */
}

int main() {
  gtthread_t thread[100];
  gtthread_init(500);
  int retvals_in[100];
  srand(time(NULL));
  gtthread_mutex_init(&test_mutex);
  for(int i = 0; i < 100; i++)
  {
      retvals_in[i] = rand();
      gtthread_create(&thread[i],thr1,(void*) retvals_in[i]);
  }
  for(int i = 0; i < 100; i++)
  {
      gtthread_join(thread[i],NULL);
  }
  if(count == 100){
      printf("Passes mutex test\n");
  }
  else
      printf("Error with mutexes\n");

 // gtthread_yield();

  return EXIT_SUCCESS;
}
