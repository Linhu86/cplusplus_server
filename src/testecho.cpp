/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <syslog.h>

#include "msgdecoder.hpp"
#include "buffer.hpp"

#include "server.hpp"
#include "handler.hpp"
#include "response.hpp"
#include "request.hpp"
#include "utils.hpp"


inline int initsock()
{
  return 0;
}


class EchoHandler : public Handler {
  public:
    EchoHandler(){}
    virtual ~EchoHandler(){}

    // return -1 : terminate session, 0 : continue
    virtual int start(Request * request, Response * response) {
      request->setMsgDecoder(new MultiLineMsgDecoder());
      response->getReply()->getMsg()->append("Welcome to line echo server, enter 'quit' to quit.\r\n");
      return 0;
    }

    // return -1 : terminate session, 0 : continue
    virtual int handle(Request * request, Response * response) {
      MultiLineMsgDecoder * decoder = (MultiLineMsgDecoder*)request->getMsgDecoder();
      CircleQueue * queue = decoder->getQueue();

    int ret = 0;
    for( ; NULL != queue->top(); ) {
      char * line = (char*)queue->pop();

      if(0 != strcasecmp( line, "quit" )) {
        response->getReply()->getMsg()->append( line );
        response->getReply()->getMsg()->append( "\r\n" );
      } else {
        response->getReply()->getMsg()->append("Byebye\r\n");
        ret = -1;
      }

      free( line );
    }

    return ret;
  }

  virtual void error(Response * response) {}

  virtual void timeout(Response * response) {}

  virtual void close() {}
};

class EchoHandlerFactory : public HandlerFactory {
  public:
    EchoHandlerFactory() {}
    virtual ~EchoHandlerFactory() {}

    virtual Handler * create() const {
      return new EchoHandler();
    }
};

//---------------------------------------------------------

int main( int argc, char * argv[] )
{
  openlog("testecho", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

  int port = 3333;

  assert( 0 == initsock() );

  Server server("", port, new EchoHandlerFactory());
  server.setMaxConnections(100000);
  server.setReqQueueSize(10000, "Server busy!");
  server.runForever();

  return 0;
}

