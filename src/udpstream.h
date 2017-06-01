// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "util.h"
#include "multithreading.h"

void *run_ntttcp_sender_udp_stream( void *ptr );
void *run_ntttcp_sender_udp4_stream( struct ntttcp_stream_client * sc );
//void *run_ntttcp_sender_udp6_stream( struct ntttcp_stream_client * sc );

void *run_ntttcp_receiver_udp_stream( void *ptr );
void *run_ntttcp_receiver_udp4_stream( struct ntttcp_stream_server * ss  );
//void *run_ntttcp_receiver_udp6_stream( struct ntttcp_stream_server * ss  );
