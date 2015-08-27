/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <syslog.h>

#include "server.hpp"
#include "eventcb.hpp"
#include "handler.hpp"
#include "session.hpp"
#include "executor.hpp"
#include "utils.hpp"
#include "iochannel.hpp"
#include "ioutils.hpp"

#include "event_msgqueue.h"

Server :: Server(const char * bindIP, int port,
    HandlerFactory * handlerFactory)
{
  snprintf(mBindIP, sizeof(mBindIP), "%s", bindIP);
  mPort = port;
  mIsShutdown = 0;
  mIsRunning = 0;

  mHandlerFactory = handlerFactory;
  mIOChannelFactory = NULL;

  mTimeout = 600;
  mMaxThreads = 4;
  mReqQueueSize = 128;
  mMaxConnections = 256;
  mRefusedMsg = strdup( "System busy, try again later." );
}

Server :: ~Server()
{
  if(NULL != mHandlerFactory) 
    delete mHandlerFactory;

  mHandlerFactory = NULL;

  if(NULL != mIOChannelFactory)
    delete mIOChannelFactory;

  mIOChannelFactory = NULL;

  if(NULL != mRefusedMsg)
    free( mRefusedMsg );

  mRefusedMsg = NULL;
}

void Server :: setIOChannelFactory(IOChannelFactory * ioChannelFactory)
{
  mIOChannelFactory = ioChannelFactory;
}

void Server :: setTimeout(int timeout)
{
  mTimeout = timeout > 0 ? timeout : mTimeout;
}

void Server :: setMaxThreads( int maxThreads )
{
  mMaxThreads = maxThreads > 0 ? maxThreads : mMaxThreads;
}

void Server :: setMaxConnections( int maxConnections )
{
  mMaxConnections = maxConnections > 0 ? maxConnections : mMaxConnections;
}

void Server :: setReqQueueSize( int reqQueueSize, const char * refusedMsg )
{
	mReqQueueSize = reqQueueSize > 0 ? reqQueueSize : mReqQueueSize;

	if( NULL != mRefusedMsg ) free( mRefusedMsg );
	mRefusedMsg = strdup( refusedMsg );
}

void Server :: shutdown()
{
  mIsShutdown = 1;
}

int Server :: isRunning()
{
  return mIsRunning;
}

int Server :: run()
{
  int ret = -1;

  pthread_attr_t attr;
  pthread_attr_init( &attr );
  assert(pthread_attr_setstacksize(&attr, 1024 * 1024 ) == 0);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_t thread;
  ret = pthread_create( &thread, &attr, eventLoop, this );
  pthread_attr_destroy( &attr );
  if( 0 == ret ) {
    syslog( LOG_NOTICE, "Thread #%ld has been created to listen on port [%d]", thread, mPort );
  } else {
    mIsRunning = 0;
    syslog( LOG_WARNING, "Unable to create a thread for TCP server on port [%d], %s",
       mPort, strerror( errno ) ) ;
  }

  return ret;
}

void Server :: runForever()
{
  eventLoop(this);
}

thread_result_t THREAD_CALL Server :: eventLoop(void * arg)
{
  Server * server = (Server*)arg;

  server->mIsRunning = 1;

  server->start();

  server->mIsRunning = 0;

  return 0;
}

void Server :: sigHandler( int, short, void * arg )
{
  Server * server = (Server*)arg;
  server->shutdown();
}

void Server :: outputCompleted( void * arg )
{
  CompletionHandler * handler = (CompletionHandler *) ((void**)arg)[0];
  Message * msg = (Message *) ((void**)arg)[ 1 ];

  handler->completionMessage(msg);

  free(arg);
}

int Server :: start()
{
#ifdef SIGPIPE
  /* Don't die with SIGPIPE on remote read shutdown. That's dumb. */
  signal( SIGPIPE, SIG_IGN );
#endif

  int ret = 0;
  int listenFD = -1;

  ret = IOUtils::tcpListen(mBindIP, mPort, &listenFD, 0);

  if( 0 == ret ) {

    EventArg eventArg( mTimeout );

    // Clean close on SIGINT or SIGTERM.
    struct event evSigInt, evSigTerm;
    signal_set( &evSigInt, SIGINT,  sigHandler, this );
    event_base_set( eventArg.getEventBase(), &evSigInt );
    signal_add( &evSigInt, NULL);
    signal_set( &evSigTerm, SIGTERM, sigHandler, this );
    event_base_set( eventArg.getEventBase(), &evSigTerm );
    signal_add( &evSigTerm, NULL);

    AcceptArg_t acceptArg;
    memset( &acceptArg, 0, sizeof(AcceptArg_t));

    if( NULL == mIOChannelFactory ) {
      mIOChannelFactory = new DefaultIOChannelFactory();
    }
    acceptArg.mEventArg = &eventArg;
    acceptArg.mHandlerFactory = mHandlerFactory;
    acceptArg.mIOChannelFactory = mIOChannelFactory;
    acceptArg.mReqQueueSize = mReqQueueSize;
    acceptArg.mMaxConnections = mMaxConnections;
    acceptArg.mRefusedMsg = mRefusedMsg;

    struct event evAccept;
    event_set( &evAccept, listenFD, EV_READ|EV_PERSIST,
      EventCallback::onAccept, &acceptArg );
    event_base_set( eventArg.getEventBase(), &evAccept );
    event_add( &evAccept, NULL );

    Executor workerExecutor( mMaxThreads, "work" );
    Executor actExecutor( 1, "act" );
    CompletionHandler * completionHandler = mHandlerFactory->createCompletionHandler();

    /* Start the event loop. */
    while( 0 == mIsShutdown ) {
      event_base_loop( eventArg.getEventBase(), EVLOOP_ONCE );

      for( ; NULL != eventArg.getInputResultQueue()->top(); ) {
        Task * task = (Task*)eventArg.getInputResultQueue()->pop();
        workerExecutor.execute( task );
      }

      for( ; NULL != eventArg.getOutputResultQueue()->top(); ) {
        Message * msg = (Message*)eventArg.getOutputResultQueue()->pop();

        void ** arg = ( void** )malloc( sizeof( void * ) * 2 );
        arg[0] = (void*)completionHandler;
        arg[1] = (void*)msg;

        actExecutor.execute( outputCompleted, arg );
      }
    }

    delete completionHandler;

    syslog(LOG_NOTICE, "Server is shutdown.");

    event_del(&evAccept);

    signal_del(&evSigTerm);
    signal_del(&evSigInt);

    close(listenFD);
  }

  return ret;
}


