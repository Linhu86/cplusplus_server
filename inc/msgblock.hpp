#ifndef __MSGBLOCK_HPP__
#define __MSGBLOCK_HPP__

#include <stdio.h>

class MsgBlock
{
  public:
    virtual ~MsgBlock();
    virtual const void *getData() const = 0;
    virtual size_t getSize() const = 0;
};

class MsgBlockList 
{
  public:
    MsgBlockList();
    ~MsgBlockList();
 
    void reset();
    size_t getTotalSize() const;

    int getCount() const;
    int append(MsgBlock * msgBlock);
    const MsgBlock * getItem(int index) const;
    MsgBlock * takeItem(int index);

private:
    MsgBlockList(MsgBlockList &);
    MsgBlockList &operator=(MsgBlockList &);
    ArrayList * mList;
};

class BufferMsgBlock : public MsgBlock
{
  public:
    BufferMsgBlock();
    BufferMsgBlock(Buffer * buffer, int toBeOwner);
    virtual ~BufferMsgBlock();
    virtual const void * getData() const;
    virtual size_t getSize() const;
    int append(const void * buffer, size_t len = 0);

private:
    BufferMsgBlock(BufferMsgBlock &);
    BufferMsgBlock & operator=(BufferMsgBlock &);
    Buffer * mBuffer;
    int mToBeOwner;
};

class SimpleMsgBlock : public MsgBlock
{
  public:
    SimpleMsgBlock();
    SimpleMsgBlock(void * data, size_t size, int toBeOwner);
    virtual ~SimpleMsgBlock();
    virtual const void * getData() const;
    virtual size_t getSize() const;
    void setData(void * data, size_t size, int toBeOwner);

private:
    SimpleMsgBlock(SimpleMsgBlock &);
    SimpleMsgBlock & operator=(SimpleMsgBlock &);    
    void * mData;
    size_t mSize;
    int mToBeOwner;
};

#endif



