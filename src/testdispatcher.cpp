/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "dispatcher.hpp"
#include "handler.hpp"
#include "response.hpp"
#include "request.hpp"
#include "buffer.hpp"
#include "msgdecoder.hpp"
#include "eventcb.hpp"
#include "ioutils.hpp"

class EchoHandler : public Handler {
  public:
    EchoHandler(){}
    virtual ~EchoHandler(){}

    // return -1 : terminate session, 0 : continue
    virtual int start(Request * request, Response * response) {
      request->setMsgDecoder(new LineMsgDecoder());
      response->getReply()->getMsg()->append(
      "Welcome to line echo dispatcher, enter 'quit' to quit.\r\n" );

      return 0;
    }

    // return -1 : terminate session, 0 : continue
    virtual int handle(Request * request, Response * response) {
    LineMsgDecoder * decoder = (LineMsgDecoder*)request->getMsgDecoder();

    if(0 != strcasecmp((char*)decoder->getMsg(), "quit")) {
      response->getReply()->getMsg()->append((char*)decoder->getMsg());
      response->getReply()->getMsg()->append("\r\n");
      return 0;
    } else {
      response->getReply()->getMsg()->append("Byebye\r\n");
      return -1;
    }
  }

  virtual void error(Response * response) {}

  virtual void timeout(Response * response) {}

  virtual void close() {}
};

class EchoTimerHandler : public TimerHandler {
  public:
    EchoTimerHandler(){
      mCount = 1;
    }

    virtual EchoTimerHandler(){}

    // return -1 : terminate timer, 0 : continue
    virtual int handle(Response * response, struct timeval * timeout) {
    syslog(LOG_NOTICE, "time = %li, call timer handler", time(NULL));

    if(++mCount >= 10) {
      syslog(LOG_NOTICE, "stop timer");
      return -1;
    } else {
      syslog(LOG_NOTICE, "set timer to %d seconds later", mCount);
      timeout->tv_sec = mCount;
      return 0;
    }
  }

  private:
    int mCount;
};

int main(int argc, char * argv[])
{
  int port = 3333, maxThreads = 10;

  extern char *optarg;
  int c;

  while((c = getopt(argc, argv, "p:t:v")) != EOF) {
    switch (c) {
      case 'p' :
        port = atoi(optarg);
        break;
      case 't':
        maxThreads = atoi(optarg);
        break;
      case '?':
      case 'v':
        printf("Usage: %s [-p <port>] [-t <threads>]\n", argv[0]);
        exit(0);
    }
  }

  openlog("testdispatcher", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

  int maxConnections = 100, reqQueueSize = 10;
  const char * refusedMsg = "System busy, try again later.";

  int listenFd = -1;
  if(0 == IOUtils::tcpListen("", port, &listenFd)) {
    Dispatcher dispatcher(new DefaultCompletionHandler(), maxThreads);
    dispatcher.dispatch();

    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = 1;

    dispatcher.push(&timeout, new EchoTimerHandler());

    for( ; ; ) {
      struct sockaddr_in addr;
      socklen_t socklen = sizeof(addr);
      int fd = accept(listenFd, (struct sockaddr*)&addr, &socklen);

      if(fd > 0) {
        if(dispatcher.getSessionCount() >= maxConnections
          || dispatcher.getReqQueueLength() >= reqQueueSize ) {
          write(fd, refusedMsg, strlen( refusedMsg ) );
          close(fd);
        } else {
          dispatcher.push( fd, new EchoHandler() );
        }
      } else {
         break;
      }
    }
  }

  closelog();

  return 0;
}

