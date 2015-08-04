#include "msgblock.hpp"

#include "buffer.hpp"
#include "utils.hpp"


MsgBlock :: ~MsgBlock()
{
}

MsgBlockList :: MsgBlockList()
{
  mList = new ArrayList();
}


MsgBlockList :: ~MsgBlockList()
{
  for( int i = 0; i < mList->getCount(); i++ ) {
    MsgBlock * msgBlock = (MsgBlock*)mList->getItem(i);
    delete msgBlock;
  }
  delete mList;

  mList = NULL;
}

void MsgBlockList :: reset()
{
  for( ; mList->getCount() > 0; ) {
    MsgBlock * msgBlock = (MsgBlock*)mList->takeItem(ArrayList::LAST_INDEX);
    delete msgBlock;
  }
}

size_t MsgBlockList :: getTotalSize() const
{
  size_t totalSize = 0;

  for(int i = 0; i < mList->getCount(); i++) {
    MsgBlock * msgBlock = (SP_MsgBlock*)mList->getItem( i );
    totalSize += msgBlock->getSize();
  }

  return totalSize;
}

int MsgBlockList :: getCount() const
{
  return mList->getCount();
}


int MsgBlockList :: append(MsgBlock * msgBlock)
{
  return mList->append(msgBlock);
}

const MsgBlock * MsgBlockList :: getItem(int index) const
{
  return (MsgBlock*)mList->getItem(index);
}

MsgBlock *MsgBlockList :: takeItem(int index)
{
  return (MsgBlock*)mList->takeItem(index);
}


class BufferMsgBlock :: BufferMsgBlock()
{
  mBuffer = new Buffer();
  mToBeOwner = 1;
}

BufferMsgBlock :: BufferMsgBlock(Buffer * buffer, int toBeOwner)
{
  mBuffer = buffer;
  mToBeOwner = toBeOwner;
}

BufferMsgBlock :: ~SP_BufferMsgBlock()
{
  if( mToBeOwner ) delete mBuffer;
    mBuffer = NULL;
}

const void * BufferMsgBlock :: getData() const
{
  return mBuffer->getBuffer();
}

size_t BufferMsgBlock :: getSize() const
{
  return mBuffer->getSize();
}

int BufferMsgBlock :: append( const void * buffer, size_t len )
{
  return mBuffer->append( buffer, len );
}

SimpleMsgBlock :: SimpleMsgBlock()
{
  mData = NULL;
  mSize = 0;
  mToBeOwner = 0;
}

SimpleMsgBlock :: SimpleMsgBlock( void * data, size_t size, int toBeOwner )
{
  mData = data;
  mSize = size;
  mToBeOwner = toBeOwner;
}

SimpleMsgBlock :: ~SimpleMsgBlock()
{
  if( mToBeOwner && NULL != mData ) {
    free( mData );
    mData = NULL;
  }
}

const void * SimpleMsgBlock :: getData() const
{
  return mData;
}

size_t SimpleMsgBlock :: getSize() const
{
  return mSize;
}

void SimpleMsgBlock :: setData( void * data, size_t size, int toBeOwner )
{
  if( mToBeOwner && NULL != mData ) {
    free( mData );
  }

  mData = data;
  mSize = size;
  mToBeOwner = toBeOwner;
}







