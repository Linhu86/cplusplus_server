#ifndef __MSGDECODER_HPP__
#define __MSGDECODER_HPP__

class Buffer;

class MsgDecoder{
  public:
    virtual ~MsgDecoder();
    enum { eOK, eMoreData };
    virtual int decode(Buffer *inBuffer) = 0;
}

class DefaultMsgDecoder : public MsgDecoder
{
  public:
    DefaultMsgDecoder();
    virtual ~DefaultMsgDecoder();

    virtual int decode(Buffer *inBuffer);

    Buffer *getMsg();

  private:
    Buffer *mBuffer;
}

class LineMsgDecoder : public MsgDecoder
{
  public:
    LineMsgDecoder();
    virtual ~DefaultMsgDecoder();

}



#endif


