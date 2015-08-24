#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include "response.hpp"
#include "iochannel.hpp"
#include "handler.hpp"
#include "buffer.hpp"

class Session
{
   public:
    Session( Sid_t id);
    virtual ~Session();

    struct event *getReadEvent();
    struct event *getWriteEvent();

    void setHandler( Handler *handler );
    Handler *getHandler();

    void setArg( void *arg );
    void *getArg();

    Sid_t getSid();

    Buffer *getInBuffer();
    Request * getRequest();

    void setOutOffset(int offset);
    int getOutOffset();
    ArrayList * getOutList();

    enum {eNormal, eWouldExit, eExit};
    void setStatus(int status);
    int getStatus();

    int getRunning();
    void setRunning(int running);

    int getReading();
    void setReading(int reading);

    int getWriting();
    void setWriting(int writing);

    IOChannel * getIOChannel();
    void setIOChannel(IOChannel * ioChannel);

    unsigned int getTotalRead();
    void addRead( int len );

    unsigned int getTotalWrite();
    void addWrite( int len );

private:

    Session(Session &);
    Session & operator=(Session &);

    Sid_t mSid;

    struct event * mReadEvent;
    struct event * mWriteEvent;

    Handler * mHandler;
    void * mArg;

    Buffer * mInBuffer;
    Request * mRequest;

    int mOutOffset;
    ArrayList * mOutList;

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
    void put(uint32_t key, uint16_t seq, Session * session);
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

