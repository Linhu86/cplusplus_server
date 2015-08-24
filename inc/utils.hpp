#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "thread.hpp"

class ArrayList {
  public:
    static const int LAST_INDEX;

    ArrayList( int initCount = 2 );
    virtual ~ArrayList();

    int getCount() const;
    int append( void * value );
    const void * getItem( int index ) const;
    void * takeItem( int index );

    void clean();

  private:
    ArrayList( ArrayList & );
    ArrayList & operator=( ArrayList & );

    int mMaxCount;
    int mCount;
    void ** mFirst;
};

class CircleQueue {
  public:
    CircleQueue();
    virtual ~CircleQueue();

    void push( void * item );
    void * pop();
    void * top();
    int getLength();

  private:
    void ** mEntries;
    unsigned int mHead;
    unsigned int mTail;
    unsigned int mCount;
    unsigned int mMaxCount;
};

class BlockingQueue {
  public:
    BlockingQueue();
    virtual ~BlockingQueue();

    // non-blocking
    void push( void * item );

    // blocking until can pop
    void * pop();

    // non-blocking, if empty then return NULL
    void * top();

    // non-blocking
    int getLength();

  private:
    CircleQueue * mQueue;
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
};

#endif

