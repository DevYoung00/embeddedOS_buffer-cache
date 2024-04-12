CC=gcc
LDFLAGS=-lpthread
OBJS=buffer.o buffercache.o queue.o stack.o test.o
TARGET=test

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	make clean

buffer.o: buffer.c buffercache.h
buffercache.o: buffercache.c buffercache.h
queue.o: queue.c queue.h
stack.o: stack.c stack.h
test.o: test.c buffercache.h

clean:
	rm -f *.o
