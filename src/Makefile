ifndef CC
CC=gcc
endif
ifeq ($(DEBUG), 1)
CFLAGS=-Wall -W -O0 -ggdb3 -fno-omit-frame-pointer
else
CFLAGS=-Wall -W -O2 -fno-strict-aliasing
endif
ifeq ($(PREFIX),)
PREFIX=/usr/local
endif
CLIBS=-pthread
OUT=ntttcp
OBJS=ntttcp.o logger.o oscounter.o util.o throughputmanagement.o tcpstream.o udpstream.o endpointsync.o multithreading.o parameter.o main.o

$(OUT): ntttcp.o logger.o oscounter.o util.o throughputmanagement.o tcpstream.o udpstream.o endpointsync.o multithreading.o parameter.o main.o
	$(CC) $(CFLAGS) $(CLIBS) -o $(OUT) $(OBJS)
ntttcp.o: ntttcp.c
	$(CC) $(CFLAGS) -c ntttcp.c -o ntttcp.o
logger.o: logger.c
	$(CC) $(CFLAGS) -c logger.c -o logger.o
oscounter.o: oscounter.c
	$(CC) $(CFLAGS) -c oscounter.c -o oscounter.o
util.o: util.c
	$(CC) $(CFLAGS) -c util.c -o util.o
throughputmanagement.o: throughputmanagement.c
	$(CC) $(CFLAGS) -c throughputmanagement.c -o throughputmanagement.o
tcpstream.o: tcpstream.c
	$(CC) $(CFLAGS) -c tcpstream.c -o tcpstream.o
udpstream.o:udpstream.c
	$(CC) $(CFLAGS) -c udpstream.c -o udpstream.o
endpointsync.o: endpointsync.c
	$(CC) $(CFLAGS) -c endpointsync.c -o endpointsync.o
multithreading.o: multithreading.c
	$(CC) $(CFLAGS) -c multithreading.c -o multithreading.o
parameter.o: parameter.c
	$(CC) $(CFLAGS) -c parameter.c -o parameter.o
main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

.PHONY: clean
clean:
	rm -rf $(PREFIX)/bin/$(OUT)
	rm -rf *.o $(OUT)

.PHONY: install
install:
	cp $(OUT) $(PREFIX)/bin/

