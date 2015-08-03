#ifndef __THREAD_hpp__
#define __THREAD_hpp__

/// pthread

#include <pthread.h>
#include <unistd.h>

typedef void * thread_result_t;

#define THREAD_CALL
typedef thread_result_t (*thread_func_t)(void * args);

#endif


