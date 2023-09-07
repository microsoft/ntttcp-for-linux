# Makefile for your project

# Compiler and flags
CC ?= gcc
CFLAGS ?= -Wall -W -O2 -fno-strict-aliasing

# Debug flag
DEBUG ?= 0
.if ${DEBUG} == 1
	CFLAGS += -O0 -ggdb3 -fno-omit-frame-pointer
.endif
# Installation prefix
PREFIX ?= /usr/local/bin

# Libraries
CLIBS = -pthread

# Output executable
OUT = ntttcp

# Source files
SRCS = ntttcp.c logger.c oscounter.c util.c throughputmanagement.c tcpstream.c udpstream.c endpointsync.c multithreading.c parameter.c main.c epoll.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(OUT)

# Rule to build the executable
$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $(CLIBS) -o $(OUT) $(OBJS)

# Rule to compile source files
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(OUT)

# Install rule
install:
	cp $(OUT) $(PREFIX)/

