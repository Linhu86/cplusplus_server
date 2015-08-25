#include <sys/types.h>
#include <syslog.h>
#include <assert.h>

#include "executor.hpp"
#include "threadpool.hpp"

#include "utils.hpp"


Task :: ~Task()
{
}

SimpleTask :: SimpleTask(ThreadFunc_t func, void * arg, int deleteAfterRun)
{
  mFunc = func;
  mArg = arg;
  mDeleteAfterRun = deleteAfterRun;
}

SimpleTask :: ~SimpleTask()
{
}

void SimpleTask :: run()
{
  mFunc(mArg);
  if(mDeleteAfterRun) 
    delete this;
}

Executor :: Executor(int maxThreads, const char * tag)
{
  tag = NULL == tag ? "unknown" : tag;

  mThreadPool = new ThreadPool( maxThreads, tag );

  mQueue = new BlockingQueue();

  mIsShutdown = 0;

  pthread_mutex_init(&mMutex, NULL);
  pthread_cond_init(&mCond, NULL);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  assert(pthread_attr_setstacksize(&attr, 1024 * 1024) == 0);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_t thread;
  int ret = pthread_create(&thread, &attr, eventLoop, this);
  pthread_attr_destroy(&attr);
  if(0 == ret) {
    syslog(LOG_NOTICE, "[ex@%s] Thread #%ld has been created for executor", tag, thread);
  } else {
    syslog(LOG_WARNING, "[ex@%s] Unable to create a thread for executor", tag);
  }
}

Executor :: ~Executor()
{
  shutdown();

  while(2 != mIsShutdown) {
    pthread_mutex_lock(&mMutex);
    pthread_cond_wait(&mCond, &mMutex);
    pthread_mutex_unlock(&mMutex);
  }

  pthread_mutex_destroy( &mMutex );
  pthread_cond_destroy( &mCond);

  delete mThreadPool;
  mThreadPool = NULL;

  delete mQueue;
  mQueue = NULL;
}

void Executor :: shutdown()
{
  pthread_mutex_lock(&mMutex);
  if(0 == mIsShutdown) {
    mIsShutdown = 1;

  // signal the event loop to wake up
    execute(worker, NULL);
  }
  pthread_mutex_unlock(&mMutex);
}

thread_result_t THREAD_CALL Executor :: eventLoop(void * arg)
{
  Executor * executor = (Executor *)arg;

  while(0 == executor->mIsShutdown) {
    void * queueData = executor->mQueue->pop();

    if(executor->mThreadPool->getMaxThreads() > 1) {
      if(0 != executor->mThreadPool->dispatch(worker, queueData)) {
        worker(queueData);
      }
    } else {
      worker(queueData);
    }
  }

  pthread_mutex_lock(&executor->mMutex);
  executor->mIsShutdown = 2;
  pthread_cond_signal(&executor->mCond);
  pthread_mutex_unlock(&executor->mMutex);

  return 0;
}

void Executor :: worker(void * arg)
{
  if(NULL != arg) {
    Task * task = (Task *)arg;
    task->run();
  }
}

void Executor :: execute(Task * task)
{
  mQueue->push(task);
}

void Executor :: execute(void (* func)(void *), void * arg)
{
  SimpleTask * task = new SimpleTask(func, arg, 1);
  execute(task);
}

int Executor :: getQueueLength()
{
  return mQueue->getLength();
}




