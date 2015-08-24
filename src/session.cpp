#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <event.h>

#include "session.hpp"
#include "handler.hpp"
#include "buffer.hpp"
#include "utils.hpp"
#include "request.hpp"
#include "iochannel.hpp"

typedef struct tag_SessionEntry {
  uint16_t mSeq;
  uint16_t mNext;
  Session * mSession;
} SessionEntry;

SessionManager :: SessionManager()
{
  mFreeCount = 0;
  mFreeList = 0;
  mCount = 0;
  memset(mArray, 0, sizeof(mArray));
}

SessionManager :: ~SessionManager()
{
  for(int i = 0; i<(int)(sizeof(mArray)/sizeof(mArray[0])); i++) {
    SessionEntry_t * list = mArray[i];
    if(NULL != list) {
      SessionEntry_t * iter = list;
      for(int i = 0; i<eColPerRow; i++, iter++) {
        if(NULL != iter->mSession) {
          delete iter->mSession;
          iter->mSession = NULL;
        }
      }
      free(list);
    }
  }
  memset(mArray, 0, sizeof(mArray));
}

uint32_t SessionManager :: allocKey(uint16_t * seq)
{
  uint16_t key = 0;

  if(mFreeList <= 0) {
    int avail = -1;
    for(int i = 1; i < (int)(sizeof(mArray)/sizeof(mArray[0])); i++) {
      if(NULL == mArray[i]) {
        avail = i;
        break;
      }
    }

    if(avail > 0) {
      mFreeCount += eColPerRow;
      mArray[avail] = (SessionEntry_t *)calloc(eColPerRow, sizeof(SessionEntry_t));
      for(int i = eColPerRow - 1; i >= 0; i--) {
        mArray[avail][i].mNext = mFreeList;
        mFreeList = eColPerRow * avail + i;
      }
    } else {
    // add code to deal with no avail .

    }

  }

  if( mFreeList > 0 ) {
    key = mFreeList;
    --mFreeCount;

    int row = mFreeList / eColPerRow, col = mFreeList % eColPerRow;

    *seq = mArray[row][col].mSeq;
    mFreeList = mArray[row][col].mNext;
  }

  return key;
}

int SessionManager :: getCount()
{
  return mCount;
}

int SessionManager :: getFreeCount()
{
  return mFreeCount;
}

void SessionManager :: put(uint32_t key, uint16_t seq, Session * session)
{
  int row = key/eColPerRow, col = key%eColPerRow;
  assert(NULL != mArray[row]);
  SessionEntry_t * list = mArray[row];
  assert(NULL == list[col].mSession);
  assert(seq == list[col].mSeq);
  list[col].mSession = session;
  mCount++;
}

Session * SessionManager :: get(uint32_t key, uint16_t * seq)
{
  int row = key/eColPerRow, col = key%eColPerRow;
  Session * ret = NULL;
  SessionEntry_t * list = mArray[row];
  if(NULL != list) {
    ret = list[col].mSession;
    * seq = list[col].mSeq;
  } else {
    * seq = 0;
  }

  return ret;
}

Session * SessionManager :: remove(uint32_t key, uint16_t seq)
{
  int row = key/eColPerRow, col = key%eColPerRow;
  Session * ret = NULL;
  SessionEntry_t * list = mArray[row];
  if(NULL != list) {
    assert(seq == list[col].mSeq);
    ret = list[col].mSession;
    list[col].mSession = NULL;
    list[col].mSeq++;
    list[col].mNext = mFreeList;
    mFreeList = key;
    ++mFreeCount;
    mCount--;
  }

  return ret;
}

/*-----------------------------------------------------------------*/

Session :: Session(Sid_t sid)
{
  mSid = sid;
  mReadEvent = NULL;
  mWriteEvent = NULL;
  mReadEvent = (struct event*)malloc(sizeof(struct event));
  mWriteEvent = (struct event*)malloc(sizeof(struct event));
  mHandler = NULL;
  mArg = NULL;
  mInBuffer = new Buffer();
  mRequest = new Request();
  mOutOffset = 0;
  mOutList = new ArrayList();
  mStatus = eNormal;
  mRunning = 0;
  mWriting = 0;
  mReading = 0;
  mTotalRead = mTotalWrite = 0;
  mIOChannel = NULL;
}

Session :: ~Session()
{
  if(NULL != mReadEvent) 
    free(mReadEvent);

  mReadEvent = NULL;

  if(NULL != mWriteEvent) 
    free( mWriteEvent );

  mWriteEvent = NULL;

  if(NULL != mHandler) {
    delete mHandler; 
    mHandler = NULL;
  }

  delete mRequest;
  mRequest = NULL;

  delete mInBuffer;
  mInBuffer = NULL;

  delete mOutList;
  mOutList = NULL;

  if(NULL != mIOChannel) {
    delete mIOChannel;
    mIOChannel = NULL;
  }
}

struct event * Session :: getReadEvent()
{
  return mReadEvent;
}

struct event * Session :: getWriteEvent()
{
  return mWriteEvent;
}

void Session :: setHandler(Handler * handler)
{
  mHandler = handler;
}

Handler * Session :: getHandler()
{
  return mHandler;
}

void Session :: setArg(void * arg)
{
  mArg = arg;
}

void * Session :: getArg()
{
  return mArg;
}

Sid_t Session :: getSid()
{
  return mSid;
}

Buffer * Session :: getInBuffer()
{
  return mInBuffer;
}

Request * Session :: getRequest()
{
  return mRequest;
}

void Session :: setOutOffset(int offset)
{
  mOutOffset = offset;
}

int Session :: getOutOffset()
{
  return mOutOffset;
}

ArrayList * Session :: getOutList()
{
  return mOutList;
}

void Session :: setStatus(int status)
{
  mStatus = status;
}

int Session :: getStatus()
{
  return mStatus;
}

int Session :: getRunning()
{
  return mRunning;
}

void Session :: setRunning(int running)
{
  mRunning = running;
}

int Session :: getWriting()
{
  return mWriting;
}

void Session :: setWriting(int writing)
{
  mWriting = writing;
}

int Session :: getReading()
{
  return mReading;
}

void Session :: setReading( int reading )
{
  mReading = reading;
}

IOChannel * Session :: getIOChannel()
{
  return mIOChannel;
}

void Session :: setIOChannel(IOChannel * ioChannel)
{
  mIOChannel = ioChannel;
}

unsigned int Session :: getTotalRead()
{
  return mTotalRead;
}

void Session :: addRead(int len)
{
  mTotalRead += len;
}

unsigned int Session :: getTotalWrite()
{
  return mTotalWrite;
}

void Session :: addWrite(int len)
{
  mTotalWrite += len;
}


