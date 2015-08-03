CC = g++
CFLAGS = 

TARGETS = server

SOURCES = \
		  src/threadpool.cpp \
		  src/testpool.cpp

INCLUDES += -Iinc

LIBS = -lrt

all:$(TARGETS)

$(TARGETS): $(SOURCES)
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) $^ -g -o $@

clean:
	rm -rf src/*.o server

