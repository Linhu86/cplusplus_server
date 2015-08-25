#ifndef __IOCHANNEL_HPP__
#define __IOCHANNEL_HPP__

class Session;
class Buffer;

typedef struct evbuffer evbuffer_t;

class IOChannel
{
  public:
    IOChannel();
    virtual ~IOChannel();
    virtual int init(int fd) = 0;
    virtual int receive(Session *session) = 0;
    virtual int transmit(Session *session);

  protected:
    static evbuffer_t * getEvBuffer( Buffer * buffer );
    virtual int write_vec(struct iovec *iovArray, int iovSize) = 0;

};

class IOChannelFactory {
  public:
    virtual ~IOChannelFactory();
    virtual IOChannel * create() const = 0;
};


class DefaultIOChannelFactory : public IOChannelFactory {
  public:
    DefaultIOChannelFactory();
    virtual ~DefaultIOChannelFactory();

    virtual IOChannel * create() const;
};

class DefaultIOChannel : public IOChannel {
  public:
    DefaultIOChannel();
    ~DefaultIOChannel();

  virtual int init( int fd );
  virtual int receive(Session * session );

  protected:
    virtual int write_vec( struct iovec * iovArray, int iovSize );
    int mFd;
};


#endif


