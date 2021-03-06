#ifndef __DISPATCHER_HPP__
#define __DISPATCHER_HPP__

#include "thread.hpp"

class CompletionHandler;
class Handler;
class Message;
class BlockingQueue;
class TimerHandler;
class IOChannel;
class Response;
class EventArg;

class Dispatcher{
  public:
    Dispatcher(CompletionHandler * completionHandler, int maxThreads = 64);
    ~Dispatcher();

    void setTimeout( int timeout );

    int getSessionCount();
    int getReqQueueLength();

    void shutdown();
    int isRunning(); 

    int dispatch();
  
    int push(int fd, Handler *handler, int needStart = 1);
  
    int push(int fd, Handler *handler, IOChannel * ioChannel, int needStart = 1);

    int push(const struct timeval * timeout, TimerHandler * handler);

    int push(Response * response);

  private:
    int mIsShutdown;
    int mIsRunning;
    int mMaxThreads;
 
    EventArg * mEventArg;
    CompletionHandler * mCompletionHandler;

    void * mPushQueue;

    int start();

    static thread_result_t THREAD_CALL eventLoop(void * arg);

    static void onPush(void * queueData, void * arg);

    static void outputCompleted(void * arg);

    static void onTimer(int, short, void * arg);
  
    static void timer(void * arg);
};

#endif

