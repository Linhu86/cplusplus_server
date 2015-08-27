#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "thread.hpp"


class HandlerFactory;
class IOChannelFactory;

class Server {
  public:
    Server(const char *bindIP, int port, HandlerFactory *handlerFactory);
    ~Server();

    void setTimeout( int timeout );
    void setMaxConnections( int maxConnections );
    void setMaxThreads( int maxThreads );
    void setReqQueueSize (int reqQueueSize, const char *refusedMsg);
    void setIOChannelFactory(IOChannelFactory * ioChannelFactory);

    void shutdown();
    int isRunning();
    int run();
    void runForever();

  private:
    HandlerFactory * mHandlerFactory;
    IOChannelFactory * mIOChannelFactory;
  
    char mBindIP[64];
    int mPort;
    int mIsShutdown;
    int mIsRunning;
  
    int mTimeout;
    int mMaxThreads;
    int mMaxConnections;
    int mReqQueueSize;
    char * mRefusedMsg;
  
    static thread_result_t THREAD_CALL eventLoop(void * arg);
  
    int start();
  
    static void sigHandler( int, short, void * arg );
  
    static void outputCompleted( void * arg );
  };






#endif







