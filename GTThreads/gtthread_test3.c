#include <stdio.h>
#include <stdlib.h>
#include "gtthread.h"

/* Tests creation.
   Should print "Hello World!" */
void *thr1(void *in) {
  printf("Here in function thr1 start, called with argument %i!\n",(int) in);
 // fflush(stdout);
  /*for(int i = 0; i < 50; i++)
  {
  	printf("Thread %i function thr1 \n",(int) in);
  }
  */
  gtthread_t t;
  if ( (int) in < 10)
  {
      gtthread_create(&t,thr1,(void*)((int) (in + 1)));
      gtthread_join(t,NULL);
  }


  //gtthread_exit(in);
  printf("Leaving thr1 with thread  %i\n",(int) in);
  return (void *) in;
}

int main() {
    gtthread_t t1;
    gtthread_init(500);
    gtthread_create(&t1,thr1,(void*) 1);
    gtthread_join(t1,NULL);

   // gtthread_t thread[100];
   // int retvals_in[100];
    //void* retvals[100];
/*
    srand(time(NULL));
    gtthread_init(500);
    //create a hundred threads
   for(int i = 0; i < 100; i++)
   {
       retvals_in[i] = rand();
       gtthread_create(&thread[i],thr1,(void*) retvals_in[i]);
   }
   //Join all the threads
   for(int i = 0; i < 100; i++)
   {
       gtthread_join(thread[i], &retvals[i]);
   }
   //Check
   for(int i = 0;i < 100; i++)
   {
       if((int) retvals[i] != retvals_in[i])
       {
           printf("Error in thread %i, return value expected to be %i and return value is %i \n",retvals_in[i],(int) retvals[i]);
       }
   }


 // gtthread_yield();
 // */

  return EXIT_SUCCESS;
}
