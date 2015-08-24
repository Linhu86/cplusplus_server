#--------------------------------------------------------------------

CC = g++
AR = ar cru
CFLAGS = -Wall -D_REENTRANT -D_GNU_SOURCE -g -fPIC
SOFLAGS = -shared
LDFLAGS = -lstdc++ -lpthread

LINKER = $(CC)
LINT = lint -c
RM = /bin/rm -f

LIBEVENT_INCL = -I../inc
LIBEVENT_LIB  = -levent -lrt

CFLAGS  += $(LIBEVENT_INCL)
LDFLAGS += $(LIBEVENT_LIB)

#--------------------------------------------------------------------

LIBOBJS = utils.o ioutils.o iochannel.o \
	threadpool.o buffer.o handler.o \
	msgblock.o msgdecoder.o response.o request.o \
	executor.o session.o eventcb.o server.o \
	dispatcher.o

TARGET =  libserver.so libserver.a \
		testpool 

#--------------------------------------------------------------------

all: $(TARGET)

libserver.so: $(LIBOBJS)
	$(LINKER) $(SOFLAGS) $^ -o $@

libserver.a: $(LIBOBJS)
	$(AR) $@ $^

testpool: testpool.o
	$(LINKER) $^ -L. -lserver $(LDFLAGS) -o $@

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) )

#--------------------------------------------------------------------

# make rule
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@	