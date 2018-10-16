// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/socketvar.h>
#include <net/if.h>
#else
#include <sys/epoll.h>
#endif
#include <sys/event.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "util.h"
#include "multithreading.h"

size_t n_read(int fd, char *buffer, size_t total);
size_t n_write(int fd, const char *buffer, size_t total);

void *run_ntttcp_sender_tcp_stream( void *ptr );

int ntttcp_server_listen(struct ntttcp_stream_server *ss);
int ntttcp_server_kqueue(struct ntttcp_stream_server *ss);
int ntttcp_server_epoll(struct ntttcp_stream_server *ss);
int ntttcp_server_select(struct ntttcp_stream_server *ss);
void *run_ntttcp_receiver_tcp_stream( void *ptr );
