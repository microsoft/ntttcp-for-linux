// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <net/if.h>
#include "multithreading.h"
#include "util.h"

void *run_ntttcp_sender_udp_stream(void *ptr);
void *run_ntttcp_sender_udp4_stream(struct ntttcp_stream_client *sc);

void *run_ntttcp_receiver_udp_stream(void *ptr);
void *run_ntttcp_receiver_udp4_stream(struct ntttcp_stream_server *ss);
int get_interface_name_by_ip(const char *target_ip, char iface_name[]);
