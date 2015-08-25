#ifndef __MSGDECODER_HPP__
#define __MSGDECODER_HPP__

#include <stdlib.h>
#include "utils.hpp"

class Buffer;

class MsgDecoder{
  public:
    virtual ~MsgDecoder();
    enum {eOK, eMoreData};
    virtual int decode(Buffer *inBuffer) = 0;
};

class DefaultMsgDecoder : public MsgDecoder
{
  public:
    DefaultMsgDecoder();
    virtual ~DefaultMsgDecoder();

    virtual int decode(Buffer *inBuffer);

    Buffer *getMsg();

  private:
    Buffer *mBuffer;
};

class LineMsgDecoder : public MsgDecoder
{
  public:
    LineMsgDecoder();
    virtual ~LineMsgDecoder();
    virtual int decode( Buffer *inBuffer );
    const char *getMsg();

  private:
    char *mLine;

};

class CirculeQueue;

class MultiLineMsgDecoder : public MsgDecoder
{
  public:
    MultiLineMsgDecoder();
    ~MultiLineMsgDecoder(); 
    
    virtual int decode (Buffer *inBuffer );
    CircleQueue *getQueue();
    
  private:
    CircleQueue *mQueue;
};

class DotTermMsgDecoder : public MsgDecoder
{
  public:
    DotTermMsgDecoder();
    virtual ~DotTermMsgDecoder();

    virtual int decode (Buffer *inBuffer);

    const char *getMsg();

  private:
    char *mBuffer;

};

class ArrayList;

class DotTermChunkMsgDecoder : public MsgDecoder
{
  public:
    enum{ MAX_SIZE_PER_CHUNK = 1024*64 };

    DotTermChunkMsgDecoder();
    virtual ~DotTermChunkMsgDecoder();

    virtual int decode( Buffer *inBuffer );
    char *getMsg();

  private:
    ArrayList *mList;
};

#endif


