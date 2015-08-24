#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include "thread.hpp"

#include "dispatcher.hpp"

#include "eventcb.hpp"
#include "handler.hpp"
#include "session.hpp"
#include "executor.hpp"
#include "utils.hpp"
#include "iochannel.hpp"
#include "ioutils.hpp"
#include "request.hpp"

#include "event_msgqueue.h"

Dispatcher :: Dispatcher(CompletionHandler * completionHandler, int maxThreads)
{
#ifdef SIGPIPE
  /* Don't die with SIGPIPE on remote read shutdown. That's dumb. */
  signal(SIGPIPE, SIG_IGN);
#endif

  mIsShutdown = 0;

  mIsRunning = 0;

  mEventArg = new EventArg(600);

  mMaxThreads = maxThreads > 0 ? maxThreads : 4;

  mCompletionHandler = completionHandler;

  mPushQueue = msgqueue_new(mEventArg->getEventBase(), 0, onPush, mEventArg);
}

Dispatcher :: ~Dispatcher()
{
  if(0 == mIsRunning)
    sleep( 1 );

  shutdown();

  for(; mIsRunning;)
    sleep( 1 );

  //msgqueue_destroy( (struct event_msgqueue*)mPushQueue );

  delete mEventArg;
  mEventArg = NULL;
}

void Dispatcher :: setTimeout(int timeout)
{
  mEventArg->setTimeout(timeout);
}

void Dispatcher :: shutdown()
{
  mIsShutdown = 1;
}

int Dispatcher :: isRunning()
{
  return mIsRunning;
}

int Dispatcher :: getSessionCount()
{
  return mEventArg->getSessionManager()->getCount();
}

int Dispatcher :: getReqQueueLength()
{
  return mEventArg->getInputResultQueue()->getLength();
}

int Dispatcher :: dispatch()
{
  int ret = -1;

  thread_attr_t attr;
  thread_attr_init(&attr);
  assert(thread_attr_setstacksize(&attr, 1024 * 1024) == 0);
  thread_attr_setdetachstate(&attr, THREAD_CREATE_DETACHED);

  thread_t thread;
  ret = thread_create(&thread, &attr, eventLoop, this);
  thread_attr_destroy(&attr);
  if(0 == ret) {
    syslog(LOG_NOTICE, "Thread #%ld has been created for dispatcher", thread);
  } else {
    mIsRunning = 0;
    syslog(LOG_WARNING, "Unable to create a thread for dispatcher, %s", strerror(errno));
  }
  return ret;
}

thread_result_t THREAD_CALL Dispatcher :: eventLoop(void * arg)
{
  Dispatcher * dispatcher = (Dispatcher *)arg;

  dispatcher->mIsRunning = 1;

  dispatcher->start();

  dispatcher->mIsRunning = 0;

  return 0;
}

void Dispatcher :: outputCompleted(void * arg)
{
  CompletionHandler * handler = (CompletionHandler *)((void**)arg)[0];
  Message * msg = (Message *)((void**)arg)[1];
  handler->completionMessage(msg);
  free(arg);
}

int Dispatcher :: start()
{
  Executor workerExecutor(mMaxThreads, "work");
  Executor actExecutor(1, "act");

  /* Start the event loop. */
  while(0 == mIsShutdown) {
    event_base_loop(mEventArg->getEventBase(), EVLOOP_ONCE);

    for( ; NULL != mEventArg->getInputResultQueue()->top();) {
      Task * task = (Task*)mEventArg->getInputResultQueue()->pop();
      workerExecutor.execute(task);
    }

    for( ; NULL != mEventArg->getOutputResultQueue()->top();) {
      Message * msg = (Message*)mEventArg->getOutputResultQueue()->pop();
      void ** arg = (void**)malloc(sizeof(void *) * 2);
      arg[0] = (void*)mCompletionHandler;
      arg[1] = (void*)msg;
      actExecutor.execute(outputCompleted, arg);
    }
  }

  syslog(LOG_NOTICE, "Dispatcher is shutdown.");

  return 0;
}

typedef struct tag_PushArg {
  int mType;      // 0 : fd, 1 : timer

  // for push fd
  int mFd;
  Handler * mHandler;
  IOChannel * mIOChannel;
  int mNeedStart;

  // for push timer
  struct timeval mTimeout;
  struct event mTimerEvent;
  TimerHandler * mTimerHandler;
  EventArg * mEventArg;
  void * mPushQueue;
} PushArg_t;

void Dispatcher :: onPush(void * queueData, void * arg)
{
  PushArg_t * pushArg = (PushArg_t*)queueData;
  EventArg * eventArg = (EventArg*)arg;

  if(0 == pushArg->mType) {
    Sid_t sid;
    sid.mKey = eventArg->getSessionManager()->allocKey(&sid.mSeq);
    assert(sid.mKey > 0);

    Session * session = new Session(sid);

    char clientIP[32] = { 0 }; 
    {
      struct sockaddr_in clientAddr;
      socklen_t clientLen = sizeof( clientAddr );
      getpeername(pushArg->mFd, (struct sockaddr *)&clientAddr, &clientLen);
      IOUtils::inetNtoa(&(clientAddr.sin_addr), clientIP, sizeof(clientIP));
      session->getRequest()->setClientPort(ntohs(clientAddr.sin_port));
    }
    session->getRequest()->setClientIP(clientIP);


    eventArg->getSessionManager()->put(sid.mKey, sid.mSeq, session);

    session->setHandler(pushArg->mHandler);
    session->setIOChannel(pushArg->mIOChannel);
    session->setArg(eventArg);

    event_set(session->getReadEvent(), pushArg->mFd, EV_READ, EventCallback::onRead, session);
    event_set(session->getWriteEvent(), pushArg->mFd, EV_WRITE, EventCallback::onWrite, session);

    if(pushArg->mNeedStart) {
      EventHelper::doStart(session);
    } else {
      EventCallback::addEvent(session, EV_WRITE, pushArg->mFd);
      EventCallback::addEvent(session, EV_READ, pushArg->mFd);
    }

    free(pushArg);
  } else {
    event_set(&(pushArg->mTimerEvent), -1, 0, onTimer, pushArg);
    event_base_set(eventArg->getEventBase(), &( pushArg->mTimerEvent));
    event_add(&(pushArg->mTimerEvent), &(pushArg->mTimeout));
  }
}

int Dispatcher :: push(int fd, Handler * handler, int needStart)
{
  IOChannel * ioChannel = new DefaultIOChannel();
  return push(fd, handler, ioChannel, needStart);
}

int Dispatcher :: push(int fd, Handler * handler, IOChannel * ioChannel, int needStart)
{
  PushArg_t * arg = (PushArg_t*)malloc(sizeof(PushArg_t));
  arg->mType = 0;
  arg->mFd = fd;
  arg->mHandler = handler;
  arg->mIOChannel = ioChannel;
  arg->mNeedStart = needStart;

  IOUtils::setNonblock(fd);

  return msgqueue_push((struct event_msgqueue*)mPushQueue, arg);
}

void Dispatcher :: onTimer(int, short, void * arg)
{
  PushArg_t * pushArg = (PushArg_t*)arg;

  pushArg->mEventArg->getInputResultQueue()->push(new SimpleTask(timer, pushArg, 1));
}

void Dispatcher :: timer(void * arg)
{
  PushArg_t * pushArg = (PushArg_t*)arg;
  TimerHandler * handler = pushArg->mTimerHandler;
  EventArg * eventArg = pushArg->mEventArg;

  Sid_t sid;
  sid.mKey = Sid_t::eTimerKey;
  sid.mSeq = Sid_t::eTimerSeq;
  Response * response = new Response( sid );
  if(0 == handler->handle(response, &(pushArg->mTimeout))) {
    msgqueue_push((struct event_msgqueue*)pushArg->mPushQueue, arg);
  } else {
    delete pushArg->mTimerHandler;
    free( pushArg );
  }

  msgqueue_push((struct event_msgqueue*)eventArg->getResponseQueue(), response);
}

int Dispatcher :: push(const struct timeval * timeout, SP_TimerHandler * handler)
{
  PushArg_t * arg = (PushArg_t*)malloc(sizeof(PushArg_t));

  arg->mType = 1;
  arg->mTimeout = *timeout;
  arg->mTimerHandler = handler;
  arg->mEventArg = mEventArg;
  arg->mPushQueue = mPushQueue;

  return msgqueue_push((struct event_msgqueue*)mPushQueue, arg);
}

int Dispatcher :: push(Response * response)
{
  return msgqueue_push((struct event_msgqueue*)mEventArg->getResponseQueue(), response);
}

