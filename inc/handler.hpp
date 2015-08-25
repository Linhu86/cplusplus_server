#ifndef __HANDLER_HPP__
#define __HANDLER_HPP__

#include "request.hpp"

class Response;
class Message;

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
    virtual ~TimerHandler();
    virtual int handle(Response *response, struct timeval *timeout) = 0;
};


class CompletionHandler{
  public:
    virtual ~CompletionHandler();
    virtual void completionMessage(Message *msg) = 0;
};

class DefaultCompletionHandler : public CompletionHandler {
  public:
    DefaultCompletionHandler();
    ~DefaultCompletionHandler();

    virtual void completionMessage(Message * msg);
};


class HandlerFactory{
  public:
    virtual ~HandlerFactory();
    virtual Handler *create() const = 0;
    virtual CompletionHandler *createCompletionHandler() const;
};

#endif





