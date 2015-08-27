#include <stdlib.h>

#include "response.hpp"
#include "buffer.hpp"
#include "utils.hpp"
#include "msgblock.hpp"

SidList :: SidList()
{
  mList = new ArrayList();
}

SidList :: ~SidList()
{
  for(int i = 0; i < mList->getCount(); i++) {
    free((void*)mList->getItem(i));
  }

  delete mList;
  mList = NULL;
}

void SidList :: reset()
{
  for( ; mList->getCount() > 0; ) {
    free((void*)mList->takeItem(ArrayList::LAST_INDEX));
  }
}

int SidList :: getCount() const
{
  return mList->getCount();
}

void SidList :: add(Resonse_Sid_t sid)
{
  Resonse_Sid_t * p = (Resonse_Sid_t*)malloc(sizeof(Resonse_Sid_t));
  *p = sid;
  mList->append(p);
}

Resonse_Sid_t SidList :: get(int index) const
{
  Resonse_Sid_t ret = { 0, 0 };
  Resonse_Sid_t * p = (Resonse_Sid_t*)mList->getItem(index);

  if(NULL != p) 
    ret = *p;

  return ret;
}

Resonse_Sid_t SidList :: take(int index)
{
  Resonse_Sid_t ret = get(index);
  void * p = mList->takeItem(index);

  if( NULL != p ) 
    free(p);

  return ret;
}

int SidList :: find(Resonse_Sid_t sid) const
{
  for(int i = 0; i < mList->getCount(); i++) {
    Resonse_Sid_t * p = (Resonse_Sid_t*)mList->getItem(i);

  if(p->mKey == sid.mKey && p->mSeq == sid.mSeq) 
    return i;
  }

  return -1;
}


Message :: Message(int completionKey)
{
  mCompletionKey = completionKey;
  mMsg = NULL;
  mFollowBlockList = NULL;
  mToList = mSuccess = mFailure = NULL;
}

Message :: ~Message()
{
  if(NULL != mMsg) 
    delete mMsg;

  mMsg = NULL;

  if(NULL != mFollowBlockList) 
    delete mFollowBlockList;

  mFollowBlockList = NULL;

  if(NULL != mToList)
    delete mToList;

  mToList = NULL;

  if(NULL != mSuccess)
    delete mSuccess;

  mSuccess = NULL;

  if(NULL != mFailure)
    delete mFailure;

  mFailure = NULL;
}

void Message :: reset()
{
  if(NULL != mMsg)
    mMsg->reset();

  if(NULL != mFollowBlockList)
    mFollowBlockList->reset();

  if(NULL != mToList)
    mToList->reset();

  if(NULL != mSuccess)
    mSuccess->reset();

  if(NULL != mFailure)
    mFailure->reset();
}

SidList * Message :: getToList()
{
  if(NULL == mToList)
    mToList = new SidList();

  return mToList;
}

size_t Message :: getTotalSize()
{
  size_t totalSize = 0;

  if(NULL != mMsg)
    totalSize += mMsg->getSize();

  if( NULL != mFollowBlockList ) 
    totalSize += mFollowBlockList->getTotalSize();

  return totalSize;
}

Buffer * Message :: getMsg()
{
  if(NULL == mMsg)
    mMsg = new Buffer();

  return mMsg;
}

MsgBlockList * Message :: getFollowBlockList()
{
  if(NULL == mFollowBlockList)
    mFollowBlockList = new MsgBlockList();

  return mFollowBlockList;
}

SidList * Message :: getSuccess()
{
  if(NULL == mSuccess)
    mSuccess = new SidList();

  return mSuccess;
}

SidList * Message :: getFailure()
{
  if(NULL == mFailure)
    mFailure = new SidList();

  return mFailure;
}

void Message :: setCompletionKey(int completionKey)
{
	mCompletionKey = completionKey;
}

int Message :: getCompletionKey()
{
  return mCompletionKey;
}

//-------------------------------------------------------------------

Response :: Response(Resonse_Sid_t fromSid)
{
  mFromSid = fromSid;
  mReply = NULL;
  mList = new ArrayList();
  mToCloseList = NULL;
}

Response :: ~Response()
{
  for(int i = 0; i < mList->getCount(); i++) {
    delete(Message*)mList->getItem(i);
  }

  delete mList;
  mList = NULL;
  mReply = NULL;

  if( NULL != mToCloseList ) 
    delete mToCloseList;

  mToCloseList = NULL;
}

Resonse_Sid_t Response :: getFromSid() const
{
  return mFromSid;
}

Message * Response :: getReply()
{
  if(NULL == mReply) {
    mReply = new Message();
    mReply->getToList()->add(mFromSid);
    mList->append(mReply);
  }

  return mReply;
}

void Response :: addMessage(Message * msg)
{
  mList->append(msg);
}

Message *Response :: peekMessage()
{
  return (Message *)mList->getItem(0);
}

Message *Response :: takeMessage()
{
  return (Message *) mList->takeItem( 0 );
}

SidList * Response :: getToCloseList()
{
  if(NULL == mToCloseList) 
    mToCloseList = new SidList();
  return mToCloseList;
}







