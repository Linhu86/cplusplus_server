#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__

#include <sys/types.h>

#include "buffer.hpp" 
#include "msgblock.hpp"

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct tag_Sid
{
  uint32_t mKey;
  uint16_t mSeq;

  enum {
    eTimerKey = 0,
    eTimerSeq = 65535,
    ePushKey = 1,
    ePushSeq = 65535
  };
} Resonse_Sid_t;


class SidList
{
  public:
    SidList();
    ~SidList();

    void reset();
    int getCount() const;
    void add(Resonse_Sid_t sid);
    Resonse_Sid_t get(int index) const;
    Resonse_Sid_t take(int index);
    int find(Resonse_Sid_t sid) const;

  private:
    SidList(SidList &);
    SidList & operator=(SidList &);
    ArrayList * mList;
};


class Message
{
  public:
    Message(int completionKey = 0);
    ~Message();

    void reset();
    SidList * getToList();
    size_t getTotalSize();
    Buffer * getMsg();
    MsgBlockList * getFollowBlockList();
    SidList * getSuccess();
    SidList * getFailure();
    void setCompletionKey(int completionKey);
    int getCompletionKey();

  private:
    Message(Message &);
    Message & operator=(Message &);
    Buffer * mMsg;
    MsgBlockList * mFollowBlockList;
    SidList * mToList;
    SidList * mSuccess;
    SidList * mFailure;
    int mCompletionKey;    

};

class Response {
  public:
    Response(Resonse_Sid_t fromSid);
    ~Response();

    Resonse_Sid_t getFromSid() const;
    Message * getReply();
    void addMessage(Message * msg);
    Message * peekMessage();
    Message * takeMessage();
    SidList * getToCloseList();

private:
    Response(Response &);
    Response & operator=(Response &);
    Resonse_Sid_t mFromSid;
    Message * mReply;
    SidList * mToCloseList;
    ArrayList * mList;
};



#endif




