#ifndef __EXECUTOR_HPP__
#define __EXECUTOR_HPP__

#include "thread.hpp"

class ThreadPool;
class BlockingQueue;

class Task
{
  public:
    virtual ~Task();
    virtual void run() = 0;
};

class SimpleTask : public Task {
  public:
    typedef void (* ThreadFunc_t) (void *);
    SimpleTask(ThreadFunc_t func, void * arg, int deleteAfterRun);
    virtual ~SimpleTask();
    virtual void run();

private:
    ThreadFunc_t mFunc;
    void * mArg;
    int mDeleteAfterRun;
};

class Executor {
  public:
    Executor( int maxThreads, const char * tag = 0 );
    ~Executor();

    void execute(Task * task);
    void execute(void (* func)(void *), void *arg);
    int getQueueLength();
    void shutdown();

private:
    static void msgQueueCallback(void * queueData, void * arg);
    static void worker(void * arg);
    static thread_result_t THREAD_CALL eventLoop(void * arg);

    ThreadPool * mThreadPool;
    BlockingQueue * mQueue;

    int mIsShutdown;

    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
};



#endif

