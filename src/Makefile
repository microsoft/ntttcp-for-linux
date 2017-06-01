CC=gcc
CFLAGS=-Wall -W -O2 -fno-strict-aliasing
CLIBS=-pthread
OUT=ntttcp
OBJS=ntttcp.o util.o tcpstream.o udpstream.o endpointsync.o multithreading.o main.o

$(OUT): ntttcp.o util.o tcpstream.o udpstream.o endpointsync.o multithreading.o main.o
	$(CC) $(CFLAGS) $(CLIBS) -o $(OUT) $(OBJS)
ntttcp.o: ntttcp.c
	$(CC) $(CFLAGS) -c ntttcp.c -o ntttcp.o
util.o: util.c
	$(CC) $(CFLAGS) -c util.c -o util.o
tcpstream.o: tcpstream.c
	$(CC) $(CFLAGS) -c tcpstream.c -o tcpstream.o
udpstream.o:udpstream.c
	$(CC) $(CFLAGS) -c udpstream.c -o udpstream.o
endpointsync.o: endpointsync.c
	$(CC) $(CFLAGS) -c endpointsync.c -o endpointsync.o
multithreading.o: multithreading.c
	$(CC) $(CFLAGS) -c multithreading.c -o multithreading.o
main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

.PHONY: clean
clean:
	rm -rf /usr/bin/$(OUT)
	rm -rf *.o $(OUT)

.PHONY: install
install:
	cp $(OUT) /usr/bin

