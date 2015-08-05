#include <sys/types.h>
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

  thread_mutex_init(&mMutex, NULL);
  thread_cond_init(&mCond, NULL);

  thread_attr_t attr;
  thread_attr_init(&attr);
  assert(thread_attr_setstacksize(&attr, 1024 * 1024) == 0);
  thread_attr_setdetachstate(&attr, THREAD_CREATE_DETACHED);

  thread_t thread;
  int ret = thread_create(&thread, &attr, eventLoop, this);
  thread_attr_destroy(&attr);
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
    thread_mutex_lock(&mMutex);
    thread_cond_wait(&mCond, &mMutex);
    thread_mutex_unlock(&mMutex);
  }

  thread_mutex_destroy( &mMutex );
  thread_cond_destroy( &mCond);

  delete mThreadPool;
  mThreadPool = NULL;

  delete mQueue;
  mQueue = NULL;
}

void Executor :: shutdown()
{
  thread_mutex_lock(&mMutex);
  if(0 == mIsShutdown) {
    mIsShutdown = 1;

  // signal the event loop to wake up
    execute(worker, NULL);
  }
  thread_mutex_unlock(&mMutex);
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

  thread_mutex_lock(&executor->mMutex);
  executor->mIsShutdown = 2;
  thread_cond_signal(&executor->mCond);
  thread_mutex_unlock(&executor->mMutex);

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




