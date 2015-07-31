#ifndef __THREAD_hpp__
#define __THREAD_hpp__

/// pthread

#include <pthread.h>
#include <unistd.h>

typedef void * sp_thread_result_t;
typedef pthread_mutex_t sp_thread_mutex_t;
typedef pthread_cond_t  sp_thread_cond_t;
typedef pthread_t       sp_thread_t;
typedef pthread_attr_t  sp_thread_attr_t;

#define sp_thread_mutex_init     pthread_mutex_init
#define sp_thread_mutex_destroy  pthread_mutex_destroy
#define sp_thread_mutex_lock     pthread_mutex_lock
#define sp_thread_mutex_unlock   pthread_mutex_unlock

#define sp_thread_cond_init      pthread_cond_init
#define sp_thread_cond_destroy   pthread_cond_destroy
#define sp_thread_cond_wait      pthread_cond_wait
#define sp_thread_cond_signal    pthread_cond_signal

#define sp_thread_attr_init           pthread_attr_init
#define sp_thread_attr_destroy        pthread_attr_destroy
#define sp_thread_attr_setdetachstate pthread_attr_setdetachstate
#define SP_THREAD_CREATE_DETACHED     PTHREAD_CREATE_DETACHED
#define sp_thread_attr_setstacksize   pthread_attr_setstacksize

#define sp_thread_self    pthread_self
#define sp_thread_create  pthread_create

#define SP_THREAD_CALL
typedef sp_thread_result_t ( * sp_thread_func_t )( void * args );

#ifndef sp_sleep
#define sp_sleep(x) sleep(x)
#endif

#endif


