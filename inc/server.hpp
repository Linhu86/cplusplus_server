#ifndef __SERVER_HPP__
#define __SERVER_HPP__

class Server {
  public:
    Server(const char *bindIP, int port, HandlerFactory *handlerFactory);
    ~SP_Server();

    void setTimeout( int timeout );
    void setMaxConnections( int maxConnections );
    void setMaxThreads( int maxThreads );
    void setReqQueueSize (int reqQueueSize, const char *refusedMsg);
    void setIOChannelFactory( IOChannelFactory * ioChannelFactory);

    void shutdown();
    int isRunning();
    int run();
    void runForever();

  private:
    HandlerFactory *mHandlerFactory;
    
}





#endif







