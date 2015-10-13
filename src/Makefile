CC=gcc
CFLAGS=-Wall -g
CLIBS=-pthread
OUT=ntttcp
OBJS=ntttcp.o tcpstream.o util.o

$(OUT): ntttcp.o tcpstream.o util.o
	$(CC) $(CFLAGS) $(CLIBS) -o $(OUT) $(OBJS)
ntttcp.o: ntttcp.c
	$(CC) $(CFLAGS) -c ntttcp.c -o ntttcp.o
tcpstream.o: tcpstream.c
	$(CC) $(CFLAGS) -c tcpstream.c -o tcpstream.o

.PHONY: clean
clean:
	rm -rf *.o $(OUT)

.PHONY: install
install:
	cp $(OUT) /usr/bin

