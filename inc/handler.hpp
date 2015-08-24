#ifndef __HANDLER_HPP__
#define __HANDLER_HPP__

struct event;
struct timeval;

class Handler
{
  public:
    virtual ~Handler();
    virtual int start(Request *request, Response *response) = 0;
    virtual int handle(Request *request, Response *response) = 0;
    virtual void error(Response *response) = 0;
    virtual void timeout(Response *response) = 0;
    virtual void close() = 0;
};

class TimerHandler{
  public:
    virtual TimerHandler();
    virtual int handle(Response *response, struct timeval *timeout) = 0;
};


class CompletionHandler{
  public:
    virtual CompletionHandler();
    virtual void completionMessage(Message *msg) = 0;
};

class HandlerFactory{
  public:
    virtual ~HandlerFactory();
    virtual Handler *create() const = 0;
    virtual CompletionHandler *createCompletionHandler() const;
}

#endif





