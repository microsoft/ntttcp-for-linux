// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <net/if.h>
#include "multithreading.h"
#include "util.h"

int n_recv(int fd, char *buffer, size_t total);
int n_send(int fd, const char *buffer, size_t total);

void *run_ntttcp_sender_tcp_stream(void *ptr);

int ntttcp_server_listen(struct ntttcp_stream_server *ss);
int ntttcp_server_epoll(struct ntttcp_stream_server *ss);
int ntttcp_server_select(struct ntttcp_stream_server *ss);
void *run_ntttcp_receiver_tcp_stream(void *ptr);
int get_interface_name_by_ip(const char *target_ip, char iface_name[]);
