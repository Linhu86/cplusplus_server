#ifdef __IOCHANNEL_HPP__
#define __IOCHANNEL_HPP__

class Session;
class Buffer;

typedef struct evbuffer evbuffer_t;

struct iovec;

class IOChannel
{
  public:
    virtual ~IOChannel();
    virtual int init( int fd );
    virtual int receive( Session *session );
    virtual int transmit( Session *session );

  protected:
    static evbuffer_t *getEvBuffer( Buffer *buffer );
    virtual int write_vec( struct iovec *iovArray, int iovSize ) = 0;

};

class IOChannelFactory
{
  public:
    IOChannelFactory();
    ~IOChannelFactory();
    virtual int init( int fd );
    virtual int receive( Session * session );

  protected:
    virtual int write_vec( struct iovec * iovArray, int iovSize );
    int mFd;

};

#endif


