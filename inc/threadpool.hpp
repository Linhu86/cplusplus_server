#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include "thread.hpp"

typedef struct thread thread_t;

class ThreadPool
{
   public:
      typedef void (* DispatchFunc_t)(void *);
      ThreadPool(int maxThreads, const char *tag = 0);
      ~ThreadPool();

      int dispatch(DispatchFunc_t dispatchFunc, void *arg);

      int getMaxThreads;

  private:
     char *mTag;
     int mMaxThreads;
     int mIndex;
     int mTotal;
     int mIsShutdown;

     pthread_mutex_t mMainMutex;
     pthread_cond_t mIdleCond;
     pthread_cond_t mFullCond;
     pthread_cond_t mEmptyCond;

     thread_t ** mThreadList;

     static thread_result_t THREAD_CALL wrapperFunc(void *);
     int saveThread(thread_t * thread);

};

#endif






