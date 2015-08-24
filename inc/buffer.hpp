#ifndef __BUFFER_HPP__
#define __BUFFER_HPP__

typedef struct evbuffer evbuffer_t;

class Buffer {
  public:
    Buffer();
    ~Buffer();

    int append( const void *buffer, int len = 0);

    int append( const Buffer *buffer );

    int printf ( const char *fmt, ... );

    void erase ( int len );

    void reset ();

    int truncate( int len );

    void reserve( int len );

    int getCapacity();

    const void *getBuffer() const;

    const void *getRawBuffer() const;

    size_t getSize() const;

    int take(char *buffer, int len);

    char *getLine();
 
    const void *find(const void *key, size_t len);

    Buffer *take();

  private:
    evbuffer_t *mBuffer;

    friend class IOChannel;
    friend class IocpEventCallback;

};

#endif


