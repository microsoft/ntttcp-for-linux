// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <netdb.h>
#include "tcpstream.h"

/* sender side sync functions */
int create_sender_sync_socket( struct ntttcp_test_endpoint *tep );
void tell_receiver_test_exit(int sockfd);
int query_receiver_busy_state(int sockfd);
int negotiate_test_duration(int sockfd, int proposed_time);
int request_to_start(int sockfd, int request);

/* receiver side sync functions */
void *create_receiver_sync_socket( void *ptr );
