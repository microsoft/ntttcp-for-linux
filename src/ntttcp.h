// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include "const.h"
#include "logger.h"

/* maintain the test parameters accepted from user */
struct ntttcp_test
{
	bool	server_role;        /* '-s' for client */
	bool	client_role;        /* '-r' for server */

	bool	multi_clients_mode;  /* '-M' server/receiver only; will server/receiver work with multi-clients mode? */
	bool	last_client;	     /* '-L' client/sender only; indicates that this is the last client when test with multi-clients mode */

	bool	daemon;             /* '-D' for daemon mode */
	bool	use_epoll;          /* '-e' for epoll() to watch fd for events. receiver only */
	char	*mapping;           /* '-m' for connection(s),Processor,StartReceiver IP map set */
	uint	parallel;           /*    Parallel connections, from -m flag */
	int	cpu_affinity;       /*    CPU affinity, from -m flag */
	char	*bind_address;      /*    Socket binding address */
	uint	conn_per_thread;    /* '-n' for number of connections per thread. sender only */
	bool	no_synch;           /* '-N' to disable sender/receiver synch */

	int	domain;              /* default for AF_INET, or '-6' for AF_INET6 */
	int	protocol;            /* default for SOCK_STREAM for TCP, or '-u' for SOCK_DGRAM for UDP (does not support UDP for now) */
	uint	server_base_port;    /* '-p' for server listening base port */
	uint	client_base_port;    /* '-f' to pin client source port based on this */
	ulong	recv_buf_size;       /* '-b' for receive buffer option */
	ulong	send_buf_size;       /* '-B' for send buffer option */
	int	duration;            /* '-t' for total duration in sec of test (0: continuous_mode) */

	bool 	show_tcp_retransmit; /* '-R' to display TCP retransmit counters in log from /proc */
	bool	verbose;             /* '-V' for verbose logging */
};

/* manage the ntttcp sender/client, or receiver/server endpoints */
struct ntttcp_test_endpoint{
	int	endpoint_role;        /* sender or receiver role */
	struct	ntttcp_test *test;
	int	state;                /* test state */
	int	confirmed_duration;   /* test duration exchanged and confirmed by both sides */
	struct	timeval start_time;   /* timestamp of test started on this endpoint */
	struct	timeval end_time;     /* timestamp of test ended on this endpoint */
	int	synch_socket;         /* the synch channel for sender/receiver sync */
	int	total_threads;	      /* total threads, including synch thread */

	struct	ntttcp_stream_client **client_streams; /* alloc memory for this if client/sender role */
	struct	ntttcp_stream_server **server_streams; /* alloc memory for this if server/receiver role */

	pthread_t	*threads;

	int	num_remote_endpoints;
	int	remote_endpoints[MAX_REMOTE_ENDPOINTS];
};

/* manage a client test connection/stream */
struct ntttcp_stream_client{
	int	domain;
	int	protocol;
	char	*bind_address;
	uint	server_port;
	uint	client_port;
	ulong	send_buf_size;
	int	is_sync_thread;
	bool	no_synch;
	bool	continuous_mode;
	bool	verbose;

	uint64_t        total_bytes_transferred;
};

/* manage a server test connection/stream */
struct ntttcp_stream_server{
	int	domain;
	int	protocol;
	char	*bind_address;
	uint	server_port;
	ulong	recv_buf_size;
	int	is_sync_thread;
	bool	no_synch;
	bool    continuous_mode;
	bool	verbose;
	bool	use_epoll;

	int	listener;     /* this is the socket to listen on port to accept new connections */
	int	max_fd;       /* track the max socket fd */
	fd_set	read_set;     /* set of read sockets */
	fd_set	write_set;    /* set of write sockets */
	int	state;
	uint64_t	total_bytes_transferred;
};

struct ntttcp_test *new_ntttcp_test();
void default_ntttcp_test(struct ntttcp_test *test);

struct ntttcp_test_endpoint *new_ntttcp_test_endpoint(struct ntttcp_test *test, int endpoint_role);
void set_ntttcp_test_endpoint_test_continuous(struct ntttcp_test_endpoint* e);
void free_ntttcp_test_endpoint_and_test(struct ntttcp_test_endpoint* e);

struct ntttcp_stream_client *new_ntttcp_client_stream(struct ntttcp_test *test);
struct ntttcp_stream_server *new_ntttcp_server_stream(struct ntttcp_test *test);
