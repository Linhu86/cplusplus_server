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










