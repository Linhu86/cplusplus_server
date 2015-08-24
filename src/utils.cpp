#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "utils.hpp"

const int ArrayList::LAST_INDEX = -1;

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

void CircleQueue :: push(void * item)
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

void * CircleQueue :: pop()
{
  void * ret = NULL;

  if(mCount > 0) {
    ret = mEntries[mHead++];
    mHead = mHead % mMaxCount;
    mCount--;
  }

  return ret;
}

void * CircleQueue :: top()
{
  return mCount > 0 ? mEntries[mHead] : NULL;
}

int CircleQueue :: getLength()
{
  return mCount;
}


//-------------------------------------------------------------------

BlockingQueue :: BlockingQueue()
{
  mQueue = new CircleQueue();
  pthread_mutex_init(&mMutex, NULL);
  pthread_cond_init(&mCond, NULL);
}

BlockingQueue :: ~BlockingQueue()
{
  delete mQueue;
  pthread_mutex_destroy(&mMutex);
  pthread_cond_destroy(&mCond);
}

void BlockingQueue :: push(void * item)
{
  pthread_mutex_lock(&mMutex);

  mQueue->push(item);

  pthread_cond_signal(&mCond);

  pthread_mutex_unlock(&mMutex);
}

void * BlockingQueue :: pop()
{
  void * ret = NULL;

  pthread_mutex_lock(&mMutex);

  if(mQueue->getLength() == 0) {
    pthread_cond_wait(&mCond, &mMutex);
  }

  ret = mQueue->pop();

  pthread_mutex_unlock(&mMutex);

  return ret;
}

void *BlockingQueue :: top()
{
  void * ret = NULL;

  pthread_mutex_lock(&mMutex);

  ret = mQueue->top();

  pthread_mutex_unlock(&mMutex);

  return ret;
}

int BlockingQueue :: getLength()
{
  int len = 0;

  pthread_mutex_lock(&mMutex);

  len = mQueue->getLength();

  pthread_mutex_unlock(&mMutex);

  return len;
}

//-------------------------------------------------------------------

int sp_strtok(const char * src, int index, char * dest, int len,
              char delimiter, const char ** next)
{
  int ret = 0;

  const char * pos1 = src, * pos2 = NULL;

  if( isspace( delimiter ) ) delimiter = 0;

  for( ; isspace( *pos1 ); ) pos1++;

  for ( int i = 0; i < index; i++ ) {
    if( 0 == delimiter ) {
      for( ; '\0' != *pos1 && !isspace( *pos1 ); ) pos1++;
        if( '\0' == *pos1 ) pos1 = NULL;
    } else {
      pos1 = strchr ( pos1, delimiter );
    }
    if ( NULL == pos1 ) break;

    pos1++;
    for( ; isspace( *pos1 ); ) pos1++;
  }

  *dest = '\0';
  if( NULL != next ) *next = NULL;

  if ( NULL != pos1 && '\0' != * pos1 ) {
    if( 0 == delimiter ) {
      for( pos2 = pos1; '\0' != *pos2 && !isspace( *pos2 ); ) pos2++;
      if( '\0' == *pos2 ) pos2 = NULL;
    } else {
      pos2 = strchr ( pos1, delimiter );
    }
    if ( NULL == pos2 ) {
      strncpy ( dest, pos1, len );
      if ( ((int)strlen(pos1)) >= len ) ret = -2;
    } else {
      if( pos2 - pos1 >= len ) ret = -2;
      len = ( pos2 - pos1 + 1 ) > len ? len : ( pos2 - pos1 + 1 );
      strncpy( dest, pos1, len );

      for( pos2++; isspace( *pos2 ); ) pos2++;
      if( NULL != next && '\0' != *pos2 ) *next = pos2;
    }
  } else {
    ret = -1;
  }

  dest[ len - 1 ] = '\0';
  len = strlen( dest );

  // remove tailing space
  for( ; len > 0 && isspace( dest[ len - 1 ] ); ) len--;
  dest[ len ] = '\0';

  return ret;
}

char * sp_strlcpy( char * dest, const char * src, int n )
{
	strncpy( dest, src, n );
	dest[ n - 1 ] = '\0';

	return dest;
}


