#include __SESSION_HPP__
#include __SESSION_HPP__

#include "response.hpp"

class Handler;
class Buffer;
class Session;
class ArrayList;
class Request;
class IOChannel;

struct event;

class Session
{
   public:
    Session( Sid_t id);
    virtual ~Session();

    struct event *getReadEvent();
    struct event *getWriteEvent();

    void setHandler( Handler *handler );
    Handler *getHandler();

    void setarg( void *arg );
    void *getarg();

    Sid_t getSid();

    Buffer *getInBuffer();
    Request * getRequest();

    void setOutOffset( int offset );
    int getOutOffset();
    ArrayList * getOutList();

    enum { eNormal, eWouldExit, eExit };
    void setStatus( int status );
    int getStatus();

    int getRunning();
    void setRunning( int running );

    int getReading();
    void setReading( int reading );

    int getWriting();
    void setWriting( int writing );

    SP_IOChannel * getIOChannel();
    void setIOChannel( SP_IOChannel * ioChannel );

    unsigned int getTotalRead();
    void addRead( int len );

    unsigned int getTotalWrite();
    void addWrite( int len );

private:

    Session( SP_Session & );
    Session & operator=( SP_Session & );

    Sid_t mSid;

    struct event * mReadEvent;
    struct event * mWriteEvent;

    Handler * mHandler;
    void * mArg;

    Buffer * mInBuffer;
    Request * mRequest;

    int mOutOffset;
    SP_ArrayList * mOutList;

    char mStatus;
    char mRunning;
    char mWriting;
    char mReading;

    unsigned int mTotalRead, mTotalWrite;

    IOChannel * mIOChannel;

};

typedef struct tag_SessionEntry SessionEntry_t;

class SessionManager {
  public:
    SessionManager();
    ~SessionManager();

    int getCount();
    void put(uint32_t key, uint16_t seq, SP_Session * session);
    Session * get(uint32_t key, uint16_t * seq);
    Session * remove(uint32_t key, uint16_t seq);

    int getFreeCount();
    // > 0 : OK, 0 : out of memory
    uint32_t allocKey( uint16_t * seq );

  private:
    enum { eColPerRow = 1024 };
    enum { eRowNum = 64*16};
  
    int mCount;
    SessionEntry_t * mArray[ eRowNum ];

    int mFreeCount;
    uint16_t mFreeList;
};


#endif

