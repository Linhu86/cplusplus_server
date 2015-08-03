#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "utils.hpp"

const int ArrayList::LIST_INDEX = -1;

ArrayList :: ArrayList( int initCount )
{
  mMaxCount = initCount <= 0 ? 2 : initCount;
  mCount = 0;
  mFirst = (void **)malloc(sizeof(void *)*mMaxCount);
}

ArrayList :: ~ArrayList()
{
  free(mFirst);
  mFirst = NULL;
}

int ArrayList :: getCount() const
{
  return mCount;
}

int ArrayList :: append( void *value )
{
  if(NULL == value)
    return -1;

  if(mCount >= mMaxCount)
  {
    mMaxCount = ( mMaxCount*3 ) /2 + 1;
    mFirst = (void **)realloc(mFirst, sizeof(void *)*mMaxCount);
    assert(NULL != mFirst);
    memset(mFirst + mMaxCount, 0, (mMaxCount - mCount)*sizeof(void *));
  }

  mFirst[mCount++] = value;

  return 0;
}


void * ArrayList :: takeItem( int index )
{
  void *ret = NULL;

  if(LAST_INDEX == index)
    index = mCount - 1;

  if(index < 0 || index >= mCount) 
    return ret;

  ret = mFirst[index];

  mCount --;

  if((index + 1) < mMaxCount) {
      memmove( mFirst + index, mFirst + index + 1,
          (mMaxCount - index - 1) * sizeof(void *));
  } else {
      mFirst[index] = NULL;
  }
  
  return ret;
}

const void * ArrayList :: getItem(int index) const
{
  const void *ret;
  
  if(LAST_INDEX == index)
    index = mCount -1;

  if(index < 0 || index >= mCount)
    return ret;

  ret = mFirst[index];
  return ret;
}


void ArrayList:: clean()
{
  mCount = 0;
  memset(mFirst, 0, sizeof(void *) * mMaxCount);
}


CircleQueue :: CircleQueue()
{
  mMaxCount = 8;
  mEntries = (void**)malloc( sizeof( void * ) * mMaxCount );

  mHead = mTail = mCount = 0;
}

CircleQueue :: ~CircleQueue()
{
  free(mEntries);
  mEntries = NULL;
}

void CircleQueue :: push()
{
  if(mCount >= mMaxCount)
  {
    mMaxCount = (mMaxCount *3)/2 + 1;

    void **newEntries = (void **)malloc(sizeof(void *) *mMaxCount);

    unsigned int headLen = 0, tailLen = 0;
    if(mHead < mTail)
    {
      headLen = mTail - mHead;
    }
    else
    {
      headLen = mCount - mTail;
      tailLen = mTail;
    }

    memcpy(newEntries, &(mEntries[mHead]), sizeof(void *)* tailLen);

    if(tailLen)
    {
      memcpy(&(newEntries[headLen]), mEntries, sizeof(void *)*tailLen);
    }

    mHead = 0;
    mTail = headLen + tailLen;
    free(mEntries);
    mEntries = newEntries;
  }

  mEntries[mTail ++] = item;
  mTail = mTail % mMaxCount;
  mCount ++;
}





