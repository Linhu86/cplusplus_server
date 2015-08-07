#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "msgdecoder.hpp"

#include "buffer.hpp"
#include "utils.hpp"

//-------------------------------------------------------------------

MsgDecoder :: ~MsgDecoder()
{
}

//-------------------------------------------------------------------

DefaultMsgDecoder :: DefaultMsgDecoder()
{
  mBuffer = new Buffer();
}

DefaultMsgDecoder :: ~DefaultMsgDecoder()
{
  if(NULL != mBuffer)
    delete mBuffer;
  mBuffer = NULL;
}

int DefaultMsgDecoder :: decode(Buffer * inBuffer)
{
  if(inBuffer->getSize() > 0) {
    mBuffer->reset();
    mBuffer->append(inBuffer); 
    inBuffer->reset();

    return eOK;
  }

  return eMoreData;
}

Buffer * DefaultMsgDecoder :: getMsg()
{
  return mBuffer;
}

//-------------------------------------------------------------------

LineMsgDecoder :: LineMsgDecoder()
{
  mLine = NULL;
}

LineMsgDecoder :: ~LineMsgDecoder()
{
  if( NULL != mLine ) {
    free( mLine );
    mLine = NULL;
  }
}

int LineMsgDecoder :: decode(Buffer * inBuffer)
{
  if(NULL != mLine)
    free( mLine );

  mLine = inBuffer->getLine();

  return NULL == mLine ? eMoreData : eOK;
}

const char * LineMsgDecoder :: getMsg()
{
  return mLine;
}

//-------------------------------------------------------------------

MultiLineMsgDecoder :: MultiLineMsgDecoder()
{
  mQueue = new CircleQueue();
}

MultiLineMsgDecoder :: ~MultiLineMsgDecoder()
{
  for( ; NULL != mQueue->top(); ) {
    free( (void*)mQueue->pop());
  }

  delete mQueue;
  mQueue = NULL;
}

int MultiLineMsgDecoder :: decode(Buffer * inBuffer)
{
  int ret = eMoreData;

  for( ; ; ) {
    char * line = inBuffer->getLine();
    if(NULL == line)
        break;
    mQueue->push(line);
    ret = eOK;
  }

  return ret;
}

CircleQueue * MultiLineMsgDecoder :: getQueue()
{
  return mQueue;
}

//-------------------------------------------------------------------

DotTermMsgDecoder :: DotTermMsgDecoder()
{
  mBuffer = NULL;
}

DotTermMsgDecoder :: ~DotTermMsgDecoder()
{
  if(NULL != mBuffer) {
    free(mBuffer);
  }
  mBuffer = NULL;
}

int DotTermMsgDecoder :: decode(Buffer * inBuffer)
{
  if(NULL != mBuffer) {
    free( mBuffer );
    mBuffer = NULL;
  }

  const char * pos = (char*)inBuffer->find("\r\n.\r\n", 5);

  if(NULL == pos){
    pos = (char*)inBuffer->find("\n.\n", 3);
  }

  if(NULL != pos) {
    int len = pos - (char*)inBuffer->getRawBuffer();

    mBuffer = (char*)malloc(len + 1);
    memcpy(mBuffer, inBuffer->getBuffer(), len);
    mBuffer[len] = '\0';

    inBuffer->erase(len);

    /* remove with the "\n.." */
    char * src, * des;
    for(src = des = mBuffer + 1; * src != '\0';) {
      if( '.' == *src && '\n' == * (src - 1 )) src++;
        * des++ = * src++;
      }
      * des = '\0';

      if(0 == strncmp((char*)pos, "\n.\n", 3)) {
        inBuffer->erase(3);
      } else {
        inBuffer->erase( 5 );
      }
      return eOK;
    } else {
    return eMoreData;
  }
}

const char * DotTermMsgDecoder :: getMsg()
{
  return mBuffer;
}

//-------------------------------------------------------------------

DotTermChunkMsgDecoder :: DotTermChunkMsgDecoder()
{
  mList = new ArrayList();
}

DotTermChunkMsgDecoder :: ~DotTermChunkMsgDecoder()
{
  for(int i = 0; i < mList->getCount(); i++) {
    Buffer * item = (Buffer*)mList->getItem(i);
    delete item;
  }

  delete mList, mList = NULL;
}

int DotTermChunkMsgDecoder :: decode(Buffer * inBuffer)
{
  if(inBuffer->getSize() <= 0)
    return eMoreData;

  const char * pos = (char*)inBuffer->find("\r\n.\r\n", 5);

  if(NULL == pos) {
    pos = (char*)inBuffer->find( "\n.\n", 3 );
  }

  if(NULL != pos) {
    if(pos != inBuffer->getRawBuffer()) {
      int len = pos - (char*)inBuffer->getRawBuffer();
      Buffer * last = new Buffer();
      last->append(inBuffer->getBuffer(), len);
      mList->append(last);
      inBuffer->erase(len);
  }

  if(0 == strncmp((char*)pos, "\n.\n", 3)) {
    inBuffer->erase(3);
  } else {
    inBuffer->erase( 5 );
  }

  return eOK;
  } else 
  {
    if(mList->getCount() > 0) {
      char dotTerm[16] = { 0 };
      Buffer * prevBuffer = (Buffer*)mList->getItem(ArrayList::LAST_INDEX);
      pos = ((char*)prevBuffer->getRawBuffer()) + prevBuffer->getSize() - 5;
      memcpy(dotTerm, pos, 5);

      if(inBuffer->getSize() > 5) {
        memcpy(dotTerm + 5, inBuffer->getRawBuffer(), 5);
      } else {
        memcpy(dotTerm + 5, inBuffer->getRawBuffer(), inBuffer->getSize());
      }

      pos = strstr(dotTerm, "\r\n.\r\n");

      if(NULL == pos) 
        pos = strstr(dotTerm, "\n.\n");

      if(NULL != pos) {
        int prevLen = 5 - (pos - dotTerm);
        int lastLen = 5 - prevLen;

        if( 0 == strncmp((char*)pos, "\n.\n", 3))
          lastLen = 3 - prevLen;

        assert(prevLen < 5 && lastLen < 5);

        prevBuffer->truncate(prevBuffer->getSize() - prevLen);
        inBuffer->erase(lastLen);

        return eOK;
      }
    }

    if(inBuffer->getSize() >= ( MAX_SIZE_PER_CHUNK - 1024 * 4)) {
      mList->append(inBuffer->take());
    }
    inBuffer->reserve( MAX_SIZE_PER_CHUNK );
  }

  return eMoreData;
}

char * DotTermChunkMsgDecoder :: getMsg()
{
  int i = 0, totalSize = 0;
  for(i = 0; i < mList->getCount(); i++) {
    Buffer * item = (Buffer*)mList->getItem(i);
    totalSize += item->getSize();
  }

  char * ret = (char*)malloc(totalSize + 1);
  ret[totalSize] = '\0';

  char * des = ret, * src = NULL;

  for(i = 0; i < mList->getCount(); i++) {
    Buffer * item = (Buffer*)mList->getItem( i );
    memcpy(des, item->getRawBuffer(), item->getSize());
    des += item->getSize();
  }

  for(i = 0; i < mList->getCount(); i++) {
    Buffer * item = (Buffer*)mList->getItem(i);
    delete item;
  }
  mList->clean();

  /* remove with the "\n.." */
  for(src = des = ret + 1; * src != '\0'; ) {
    if('.' == *src && '\n' == * (src - 1))
        src++ ;
    * des++ = * src++;
  }
  * des = '\0';

  return ret;
}

