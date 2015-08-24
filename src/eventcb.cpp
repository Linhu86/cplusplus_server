#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <event.h>


#include "eventcb.hpp"
#include "executor.hpp"
#include "session.hpp"
#include "response.hpp"
#include "handler.hpp"
#include "buffer.hpp"
#include "msgdecoder.hpp"
#include "utils.hpp"
#include "request.hpp"
#include "msgblock.hpp"
#include "iochannel.hpp"
#include "ioutils.hpp"

#include "event_msgqueue.h"

EventArg :: EventArg(int timeout)
{
  mEventBase = (struct event_base*)event_init();

  mResponseQueue = msgqueue_new( mEventBase, 0,
  EventCallback::onResponse, this);

  mInputResultQueue = new BlockingQueue();

  mOutputResultQueue = new BlockingQueue();

  mSessionManager = new SessionManager();

  mTimeout = timeout;
}

EventArg :: ~EventArg()
{
  delete mInputResultQueue;
  delete mOutputResultQueue;

  delete mSessionManager;

  //msgqueue_destroy( (struct event_msgqueue*)mResponseQueue );
  //event_base_free( mEventBase );
}

struct event_base *EventArg :: getEventBase() const
{
  return mEventBase;
}

void * EventArg :: getResponseQueue() const
{
  return mResponseQueue;
}

BlockingQueue * EventArg :: getInputResultQueue() const
{
  return mInputResultQueue;
}

BlockingQueue * EventArg :: getOutputResultQueue() const
{
  return mOutputResultQueue;
}

SessionManager * EventArg :: getSessionManager() const
{
  return mSessionManager;
}

void EventArg :: setTimeout(int timeout)
{
  mTimeout = timeout;
}

int EventArg :: getTimeout() const
{
  return mTimeout;
}

//-------------------------------------------------------------------

void EventCallback :: onAccept(int fd, short events, void * arg)
{
  int clientFD;
  struct sockaddr_in addr;
  socklen_t addrLen = sizeof( addr );

  AcceptArg_t * acceptArg = (AcceptArg_t*)arg;
  EventArg * eventArg = acceptArg->mEventArg;

  clientFD = accept(fd, (struct sockaddr *)&addr, &addrLen);
  if( -1 == clientFD ) {
    syslog( LOG_WARNING, "accept failed" );
    return;
  }

  if(IOUtils::setNonblock(clientFD) < 0) {
    syslog(LOG_WARNING, "failed to set client socket non-blocking");
  }

  Sid_t sid;
  sid.mKey = eventArg->getSessionManager()->allocKey(&sid.mSeq);
  assert(sid.mKey > 0);

  Session * session = new Session(sid);

  char strip[32] = { 0 };
  IOUtils::inetNtoa(&(addr.sin_addr), strip, sizeof(strip));
  session->getRequest()->setClientIP(strip);
  session->getRequest()->setClientPort(ntohs(addr.sin_port));

  if(0 == getsockname(clientFD, (struct sockaddr*)&addr, &addrLen)) {
    IOUtils::inetNtoa(&(addr.sin_addr), strip, sizeof(strip));
    session->getRequest()->setServerIP(strip);
  }

  if(NULL != session) {
    eventArg->getSessionManager()->put(sid.mKey, sid.mSeq, session);

    session->setHandler(acceptArg->mHandlerFactory->create());
    session->setIOChannel(acceptArg->mIOChannelFactory->create());
    session->setArg(eventArg);

    event_set(session->getReadEvent(), clientFD, EV_READ, onRead, session);
    event_set(session->getWriteEvent(), clientFD, EV_WRITE, onWrite, session);

    if(eventArg->getSessionManager()->getCount() > acceptArg->mMaxConnections
       || eventArg->getInputResultQueue()->getLength() >= acceptArg->mReqQueueSize ) {
      syslog( LOG_WARNING, "System busy, session.count %d [%d], queue.length %d [%d]",
      eventArg->getSessionManager()->getCount(), acceptArg->mMaxConnections,
      eventArg->getInputResultQueue()->getLength(), acceptArg->mReqQueueSize );

      Message * msg = new Message();
      msg->getMsg()->append( acceptArg->mRefusedMsg );
      msg->getMsg()->append( "\r\n" );
      session->getOutList()->append( msg );
      session->setStatus( Session::eExit );

      addEvent( session, EV_WRITE, clientFD );
    } else {
       EventHelper::doStart( session );
    }
  } else {
    eventArg->getSessionManager()->remove( sid.mKey, sid.mSeq );
    close( clientFD );
    syslog( LOG_WARNING, "Out of memory, cannot allocate session object!" );
  }
}

void EventCallback :: onRead(int fd, short events, void * arg)
{
  Session * session = (Session*)arg;

  session->setReading(0);

  Sid_t sid = session->getSid();

  if(EV_READ & events) {
    int len = session->getIOChannel()->receive( session );

    if(len > 0) {
      session->addRead( len );
      if( 0 == session->getRunning() ) {
        MsgDecoder * decoder = session->getRequest()->getMsgDecoder();
        if( MsgDecoder::eOK == decoder->decode( session->getInBuffer() ) ) {
          EventHelper::doWork( session );
        }
      }
      addEvent( session, EV_READ, -1 );
    } else {
      int saved = errno;

      if( 0 != errno ) {
        syslog( LOG_WARNING, "session(%d.%d) read error, errno %d, status %d",
        sid.mKey, sid.mSeq, errno, session->getStatus() );
      }

      if( EAGAIN != saved ) {
        if( 0 == session->getRunning() ) {
          EventHelper::doError( session );
        } else {
          syslog( LOG_NOTICE, "session(%d.%d) busy, process session error later",
            sid.mKey, sid.mSeq );
          // If this session is running, then onResponse will add write event for this session.
          // It will be processed as write fail at the last. So no need to re-add event here.
        }
      } else {
        addEvent( session, EV_READ, -1 );
      }
    }
  } else {
    if( 0 == session->getRunning() ) {
       EventHelper::doTimeout( session );
    } else {
      syslog( LOG_NOTICE, "session(%d.%d) busy, process session timeout later",
        sid.mKey, sid.mSeq );
      // If this session is running, then onResponse will add write event for this session.
      // It will be processed as write fail at the last. So no need to re-add event here.
    }
  }
}

void EventCallback :: onWrite(int fd, short events, void * arg)
{
    Session * session = (Session*)arg;

    session->setWriting( 0 );

    Sid_t sid = session->getSid();

    if( EV_WRITE & events ) {
      int ret = 0;

    if( session->getOutList()->getCount() > 0 ) {
      int len = session->getIOChannel()->transmit( session );
      if( len > 0 ) {
        session->addWrite( len );
        if( session->getOutList()->getCount() > 0 ) {
          // left for next write event
          addEvent( session, EV_WRITE, -1 );
        }
      } else {
        if( EAGAIN != errno ) {
          ret = -1;
          if( 0 == session->getRunning() ) {
            syslog( LOG_NOTICE, "session(%d.%d) write error, errno %d, status %d, count %d",
                sid.mKey, sid.mSeq, errno, session->getStatus(), session->getOutList()->getCount() );
            EventHelper::doError( session );
          } else {
            syslog( LOG_NOTICE, "session(%d.%d) busy, process session error later, errno [%d]",
                sid.mKey, sid.mSeq, errno );
            // If this session is running, then onResponse will add write event for this session.
            // It will be processed as write fail at the last. So no need to re-add event here.
          }
        } else {
          addEvent( session, EV_WRITE, -1 );
        }
      }
    }

    if( 0 == ret && session->getOutList()->getCount() <= 0 ) {
      if( Session::eExit == session->getStatus() ) {
        ret = -1;
        if( 0 == session->getRunning() ) {
          syslog( LOG_DEBUG, "session(%d.%d) normal exit", sid.mKey, sid.mSeq );
          EventHelper::doClose( session );
        } else {
          syslog( LOG_NOTICE, "session(%d.%d) busy, terminate session later",
            sid.mKey, sid.mSeq );
          // If this session is running, then onResponse will add write event for this session.
          // It will be processed as write fail at the last. So no need to re-add event here.
        }
      }
    } 

    if( 0 == ret ) {
      if( 0 == session->getRunning() ) {
        MsgDecoder * decoder = session->getRequest()->getMsgDecoder();
        if(MsgDecoder::eOK == decoder->decode( session->getInBuffer() ) ) {
            EventHelper::doWork( session );
        }
      } else {
        // If this session is running, then onResponse will add write event for this session.
        // So no need to add write event here.
      }
    }
  } else {
    if( 0 == session->getRunning() ) {
      EventHelper::doTimeout( session );
    } else {
      syslog( LOG_NOTICE, "session(%d.%d) busy, process session timeout later",
        sid.mKey, sid.mSeq );
      // If this session is running, then onResponse will add write event for this session.
      // It will be processed as write fail at the last. So no need to re-add event here.
    }
  }
}

void EventCallback :: onResponse(void * queueData, void * arg)
{
  Response * response = (Response*)queueData;
  EventArg * eventArg = (EventArg*)arg;
  SessionManager * manager = eventArg->getSessionManager();

  Sid_t fromSid = response->getFromSid();
  uint16_t seq = 0;

  if( ! EventHelper::isSystemSid( &fromSid ) ) {
    Session * session = manager->get( fromSid.mKey, &seq );
    if( seq == fromSid.mSeq && NULL != session ) {
      if( Session::eWouldExit == session->getStatus() ) {
        session->setStatus( SP_Session::eExit );
      }

      if( Session::eNormal != session->getStatus() ) {
        event_del( session->getReadEvent() );
      }

      // always add a write event for sender, 
      // so the pending input can be processed in onWrite
      addEvent( session, EV_WRITE, -1 );
      addEvent( session, EV_READ, -1 );
    } else {
      syslog( LOG_WARNING, "session(%d.%d) invalid, unknown FROM",
        fromSid.mKey, fromSid.mSeq );
    }
  }

  for( Message * msg = response->takeMessage();
    NULL != msg; msg = response->takeMessage() ) {

    SidList * sidList = msg->getToList();

    if( msg->getTotalSize() > 0 ) {
      for( int i = sidList->getCount() - 1; i >= 0; i-- ) {
        Sid_t sid = sidList->get( i );
        Session * session = manager->get( sid.mKey, &seq );
        if( seq == sid.mSeq && NULL != session ) {
          if( 0 != memcmp( &fromSid, &sid, sizeof( sid ) )
              && Session::eExit == session->getStatus() ) {
            sidList->take( i );
            msg->getFailure()->add( sid );
            syslog( LOG_WARNING, "session(%d.%d) would exit, invalid TO", sid.mKey, sid.mSeq );
          } else {
            session->getOutList()->append( msg );
              addEvent( session, EV_WRITE, -1 );
          }
        } else {
          sidList->take( i );
          msg->getFailure()->add( sid );
          syslog( LOG_WARNING, "session(%d.%d) invalid, unknown TO", sid.mKey, sid.mSeq );
        }
      }
    } else {
      for( ; sidList->getCount() > 0; ) {
        msg->getFailure()->add( sidList->take( SP_ArrayList::LAST_INDEX ) );
      }
    }
    if( msg->getToList()->getCount() <= 0 ) {
      EventHelper::doCompletion( eventArg, msg );
    }
  }

  for( int i = 0; i < response->getToCloseList()->getCount(); i++ ) {
    Sid_t sid = response->getToCloseList()->get( i );
    Session * session = manager->get( sid.mKey, &seq );
    if( seq == sid.mSeq && NULL != session ) {
      session->setStatus( SP_Session::eExit );
      addEvent( session, EV_WRITE, -1 );
    } else {
      syslog( LOG_WARNING, "session(%d.%d) invalid, unknown CLOSE", sid.mKey, sid.mSeq );
    }
  }

  delete response;
}

void EventCallback :: addEvent(Session * session, short events, int fd)
{
  EventArg * eventArg = (EventArg*)session->getArg();

  if((events & EV_WRITE) && 0 == session->getWriting()) {
    session->setWriting( 1 );

    if( fd < 0 ) fd = EVENT_FD( session->getWriteEvent() );

    event_set( session->getWriteEvent(), fd, events, onWrite, session );
    event_base_set( eventArg->getEventBase(), session->getWriteEvent() );

    struct timeval timeout;
    memset( &timeout, 0, sizeof( timeout ) );
    timeout.tv_sec = eventArg->getTimeout();
    event_add( session->getWriteEvent(), &timeout );
  }

  if( events & EV_READ && 0 == session->getReading() ) {
    session->setReading( 1 );

    if( fd < 0 ) fd = EVENT_FD( session->getWriteEvent() );

    event_set( session->getReadEvent(), fd, events, onRead, session );
    event_base_set( eventArg->getEventBase(), session->getReadEvent() );

    struct timeval timeout;
    memset( &timeout, 0, sizeof( timeout ) );
    timeout.tv_sec = eventArg->getTimeout();
    event_add( session->getReadEvent(), &timeout );
  }
}

//-------------------------------------------------------------------

int EventHelper :: isSystemSid(Sid_t * sid)
{
  return (sid->mKey == SP_Sid_t::eTimerKey && sid->mSeq == Sid_t::eTimerSeq)
    || (sid->mKey == SP_Sid_t::ePushKey && sid->mSeq == Sid_t::ePushSeq);
}

void EventHelper :: doWork(Session * session)
{
  if(Session::eNormal == session->getStatus() ) {
    session->setRunning( 1 );
    EventArg * eventArg = (EventArg*)session->getArg();
    eventArg->getInputResultQueue()->push( new SimpleTask( worker, session, 1 ) );
  } else {
    Sid_t sid = session->getSid();

    char buffer[ 16 ] = { 0 };
    session->getInBuffer()->take( buffer, sizeof( buffer ) );
    syslog( LOG_WARNING, "session(%d.%d) status is %d, ignore [%s...] (%dB)",
        sid.mKey, sid.mSeq, session->getStatus(), buffer, (int)session->getInBuffer()->getSize() );
    session->getInBuffer()->reset();
  }
}

void EventHelper :: worker( void * arg )
{
  Session * session = (Session*)arg;
  Handler * handler = session->getHandler();
  EventArg * eventArg = (EventArg *)session->getArg();

  Response * response = new Response( session->getSid() );
  if( 0 != handler->handle( session->getRequest(), response ) ) {
    session->setStatus( Session::eWouldExit );
  }

  session->setRunning( 0 );

  msgqueue_push( (struct event_msgqueue*)eventArg->getResponseQueue(), response );
}

void EventHelper :: doError( Session * session )
{
  EventArg * eventArg = (EventArg *)session->getArg();

  event_del( session->getWriteEvent() );
  event_del( session->getReadEvent() );

  Sid_t sid = session->getSid();

  ArrayList * outList = session->getOutList();
  for( ; outList->getCount() > 0; ) {
    Message * msg = ( Message * ) outList->takeItem( ArrayList::LAST_INDEX );

    int index = msg->getToList()->find( sid );
    if( index >= 0 ) msg->getToList()->take( index );
      msg->getFailure()->add( sid );

    if( msg->getToList()->getCount() <= 0 ) {
      doCompletion( eventArg, msg );
    }
  }

  // remove session from SessionManager, onResponse will ignore this session
  eventArg->getSessionManager()->remove( sid.mKey, sid.mSeq );

  eventArg->getInputResultQueue()->push(new SimpleTask( error, session, 1));
}

void EventHelper :: error( void * arg )
{
  Session * session = (Session *)arg;
  EventArg * eventArg = (EventArg*)session->getArg();

  Sid_t sid = session->getSid();

  Response * response = new Response( sid );
  session->getHandler()->error( response );

  msgqueue_push((struct event_msgqueue*)eventArg->getResponseQueue(), response );

  syslog( LOG_WARNING, "session(%d.%d) error, r %d(%d), w %d(%d), i %d, o %d, s %d(%d)",
    sid.mKey, sid.mSeq, session->getTotalRead(), session->getReading(),
    session->getTotalWrite(), session->getWriting(),
    (int)session->getInBuffer()->getSize(), session->getOutList()->getCount(),
    eventArg->getSessionManager()->getCount(), eventArg->getSessionManager()->getFreeCount() );

  // onResponse will ignore this session, so it's safe to destroy session here
  session->getHandler()->close();
  close(EVENT_FD(session->getWriteEvent()));
  delete session;
}

void EventHelper :: doTimeout(Session * session)
{
  EventArg * eventArg = (EventArg*)session->getArg();
  event_del(session->getWriteEvent());
  event_del(session->getReadEvent());

  Sid_t sid = session->getSid();

  ArrayList * outList = session->getOutList();
  for( ; outList->getCount() > 0; ) {
    Message * msg = ( Message * ) outList->takeItem( ArrayList::LAST_INDEX );
    int index = msg->getToList()->find( sid );
    if( index >= 0 ) msg->getToList()->take( index );
      msg->getFailure()->add( sid );

      if(msg->getToList()->getCount() <= 0) {
        doCompletion(eventArg, msg);
    }
  }

  // remove session from SessionManager, onResponse will ignore this session
  eventArg->getSessionManager()->remove(sid.mKey, sid.mSeq);

  eventArg->getInputResultQueue()->push( new SimpleTask( timeout, session, 1 ) );
}

void EventHelper :: timeout(void * arg)
{
  Session * session = (Session *)arg;
  EventArg * eventArg = (EventArg*)session->getArg();

  Sid_t sid = session->getSid();

  Response * response = new Response( sid );
  session->getHandler()->timeout( response );
  msgqueue_push((struct event_msgqueue*)eventArg->getResponseQueue(), response);

  syslog(LOG_WARNING, "session(%d.%d) timeout, r %d(%d), w %d(%d), i %d, o %d, s %d(%d)",
    sid.mKey, sid.mSeq, session->getTotalRead(), session->getReading(),
    session->getTotalWrite(), session->getWriting(),
    (int)session->getInBuffer()->getSize(), session->getOutList()->getCount(),
    eventArg->getSessionManager()->getCount(), eventArg->getSessionManager()->getFreeCount() );

  // onResponse will ignore this session, so it's safe to destroy session here
  session->getHandler()->close();
  close(EVENT_FD(session->getWriteEvent()));
  delete session;
}

void EventHelper :: doClose(Session * session)
{
  EventArg * eventArg = (EventArg*)session->getArg();

  event_del( session->getWriteEvent() );
  event_del( session->getReadEvent() );

  Sid_t sid = session->getSid();

  eventArg->getSessionManager()->remove(sid.mKey, sid.mSeq);

  eventArg->getInputResultQueue()->push(new SimpleTask(myclose, session, 1));
}

void EventHelper :: myclose(void * arg)
{
  Session * session = (Session *)arg;
  EventArg * eventArg = (EventArg*)session->getArg();
  Sid_t sid = session->getSid();

  syslog( LOG_DEBUG, "session(%d.%d) close, r %d(%d), w %d(%d), i %d, o %d, s %d(%d)",
      sid.mKey, sid.mSeq, session->getTotalRead(), session->getReading(),
      session->getTotalWrite(), session->getWriting(),
      (int)session->getInBuffer()->getSize(), session->getOutList()->getCount(),
      eventArg->getSessionManager()->getCount(), eventArg->getSessionManager()->getFreeCount() );

  session->getHandler()->close();
  close( EVENT_FD( session->getWriteEvent() ) );
  delete session;
}

void EventHelper :: doStart( Session * session )
{
  session->setRunning( 1 );
  EventArg * eventArg = (EventArg*)session->getArg();
  eventArg->getInputResultQueue()->push( new SimpleTask( start, session, 1 ) );
}

void EventHelper :: start( void * arg )
{
  Session * session = ( Session * )arg;
  EventArg * eventArg = (EventArg*)session->getArg();

  IOChannel * ioChannel = session->getIOChannel();

  int initRet = ioChannel->init( EVENT_FD( session->getWriteEvent() ) );

  // always call SP_Handler::start
  Response * response = new Response( session->getSid() );
  int startRet = session->getHandler()->start( session->getRequest(), response );

  int status = Session::eWouldExit;

  if( 0 == initRet ) {
    if( 0 == startRet ) status = Session::eNormal;
  } else {
    delete response;
    // make an empty response
    response = new Response( session->getSid() );
  }

  session->setStatus( status );
  session->setRunning( 0 );
  msgqueue_push( (struct event_msgqueue*)eventArg->getResponseQueue(), response );
}

void SP_EventHelper :: doCompletion(EventArg * eventArg, PMessage * msg )
{
	eventArg->getOutputResultQueue()->push( msg );
}


