#ifndef __EVENTCB_HPP__
#define __EVENTCB_HPP__

class BlockingQueue;
class SessionManager;
class Session;
class HandlerFactory;
class Message;
class HandlerFactory;
class IOChannelFactory;

typedef struct tag_Sid Event_Sid_t;

class EventArg 
{
  public:
    EventArg(int timeout);
    ~EventArg();
      
    struct event_base * getEventBase() const;
    void * getResponseQueue() const;
    BlockingQueue * getInputResultQueue() const;
    BlockingQueue * getOutputResultQueue() const;
    SessionManager * getSessionManager() const;
      
    void setTimeout( int timeout );
    int getTimeout() const;
      
  private:
    struct event_base * mEventBase;
    void * mResponseQueue;  
    BlockingQueue * mInputResultQueue;
    BlockingQueue * mOutputResultQueue;
    SessionManager * mSessionManager;
    int mTimeout;
};

typedef struct tag_AcceptArg {
  EventArg * mEventArg;
  HandlerFactory * mHandlerFactory;
  IOChannelFactory * mIOChannelFactory;
  int mReqQueueSize;
  int mMaxConnections;
  char * mRefusedMsg;
} AcceptArg_t;

class EventCallback {
  public:
    static void onAccept(int fd, short events, void * arg);
    static void onRead(int fd, short events, void * arg);
    static void onWrite(int fd, short events, void * arg);
    static void onResponse(void * queueData, void * arg);
    static void addEvent(Session * session, short events, int fd);

  private:
    EventCallback();
    ~EventCallback();
};

class EventHelper {
  public:
    static void doStart(Session * session);
    static void start(void * arg);
    static void doWork(Session * session);
    static void worker(void * arg);
    static void doError(Session * session);
    static void error(void * arg);
    static void doTimeout(Session * session);
    static void timeout(void * arg);
    static void doClose(Session * session);
    static void myclose(void * arg);
    static void doCompletion(EventArg * eventArg, Message * msg);
    static int isSystemSid(Event_Sid_t * sid);

  private:
    EventHelper();
    ~EventHelper();
};



#endif


