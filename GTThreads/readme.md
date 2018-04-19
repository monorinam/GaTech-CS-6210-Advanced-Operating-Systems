# GTThreads: User Level Threading Library
It is a POSIX compliant thread library that implements the inner functionality of pthread functions.
It uses a priority round robin scheduler to run the threads.
The library runs in user mode purely and uses ucontext.h for user context switching.
The library also contains the implementation of mutex synchronization primitive to enable concurrency.
The API functions are defined in gtthreads.h
The API and skeleton code are obtained from CS6210 Devbox https://github.com/omscs-georgia-tech/cs6210-devbox

