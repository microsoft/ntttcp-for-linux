// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define __STDC_FORMAT_MACROS
#pragma once

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
	bool	exit_after_done;    /* exit receiver after test done. use '-H' to hold receiver always running. receiver only */
	char	*mapping;           /* '-m' for connection(s),Processor,StartReceiver IP map set */
	uint	server_ports;       /*    How many ports opening in receiver side, from -m flag, or -P flag */
	int	cpu_affinity;       /*    CPU affinity, from -m flag */
	char	*bind_address;      /*    Socket binding address */
	uint	threads_per_server_port;  /* '-n' for number of threads per each server port. sender only */
	uint	conns_per_thread;   /* '-l' for number of connections in each sender thread. sender only */
	bool	no_synch;           /* '-N' to disable sender/receiver synch */

	int	domain;              /* default for AF_INET, or '-6' for AF_INET6 */
	int	protocol;            /* default for SOCK_STREAM for TCP, or '-u' for SOCK_DGRAM for UDP (does not support UDP for now) */
	uint	server_base_port;    /* '-p' for server listening base port */
	uint	client_base_port;    /* '-f' to pin client source port based on this */
	ulong	recv_buf_size;       /* '-b' for receive buffer option */
	ulong	send_buf_size;       /* '-b' for send buffer option */
	long	bandwidth_limit;     /* '-B' for test bandwidth limit implemented by this tool */
	long	fq_rate_limit;       /* '--fq-rate-limit' for rate limitation implemented by Fair Queue traffic policing (FQ) */

	int	warmup;              /* '-W' for test warm-up time in sec */
	int	duration;            /* '-t' for total duration in sec of test (0: continuous_mode) */
	int	cooldown;            /* '-C' for test cool-down time in sec */

	bool 	show_tcp_retransmit;      /* '-R' to display TCP retransmit counters in log from /proc */
	char	*show_interface_packets;  /* '-K' to show number of packets tx/rx through the interface */
	char	*show_dev_interrupts;     /* '-I' to show number of interrupts of devices */
	bool	save_xml_log;             /* '-x' to save output to XML file */
	char	*xml_log_filename;        /* the xml log file name */
	bool	save_console_log;         /* '-O' to capture console log to plain text file */
	char    *console_log_filename;    /* the console log file name */
	bool    save_json_log;             /* '-J' to save output to json file */
	char    *json_log_filename;       /* the json log file name */

	bool	quiet;               /* '-Q' for quiet logging */
	bool	verbose;             /* '-V' for verbose logging */
};

/* manage the ntttcp sender/client, or receiver/server endpoints */
struct ntttcp_test_endpoint{
	int	endpoint_role;        /* sender or receiver role */
	struct	ntttcp_test *test;
	int	state;                /* test state */
	int	negotiated_test_cycle_time;   /* test cycle time (warmup + duration + cooldown) exchanged and confirmed by both sides */
	struct	timeval start_time;   /* timestamp of test started on this endpoint */
	struct	timeval end_time;     /* timestamp of test ended on this endpoint */
	int	synch_socket;         /* the synch channel for sender/receiver sync */
	unsigned int	total_threads;		/* total threads, including synch thread */
	bool    receiver_exit_after_done;	/* the receiver will exit after test done, or not */

	struct	ntttcp_stream_client **client_streams;	/* alloc memory for this if client/sender role */
	struct	ntttcp_stream_server **server_streams;	/* alloc memory for this if server/receiver role */
	pthread_t	*threads;			/* linux threads created to transfer test data */

	bool	running_tty;				/* print log to tty, or redirect it to a file */
	struct	ntttcp_test_endpoint_results	*results;	/* test results */

	/* to support testing with multiple senders */
	int	num_remote_endpoints; /* number to test client/sender endpoints */
	int	remote_endpoints[MAX_REMOTE_ENDPOINTS]; /* list of the TCP listeners of those endpoints */
};

/* manage a client test thread (one or multiple socket connections) */
struct ntttcp_stream_client{
	struct	ntttcp_test_endpoint *endpoint;
	int	domain;
	int	protocol;
	char	*bind_address;
	uint	server_port;
	uint	client_port;
	uint	num_connections;
	ulong	send_buf_size;
	ulong   sc_bandwidth_limit_bytes;	/* the bandwidth limit per stream client (thread) */
	ulong	socket_fq_rate_limit_bytes;	/* the fq rate limit per socket (connection) */
	bool    hold_on;			/* hold on sending packets on this stream client because of bandwidth limit */
	bool	is_sync_thread;
	bool	no_synch;
	bool	continuous_mode;
	bool	verbose;

	uint    average_rtt;
	uint	  num_conns_created;
	uint64_t  total_bytes_transferred;
};

/* manage a server test thread (one socket on one port) */
struct ntttcp_stream_server{
	struct  ntttcp_test_endpoint *endpoint;
	int	domain;
	int	protocol;
	char	*bind_address;
	uint	server_port;
	ulong	recv_buf_size;
	bool	is_sync_thread;
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

struct ntttcp_stream_client *new_ntttcp_client_stream(struct ntttcp_test_endpoint *endpoint);
struct ntttcp_stream_server *new_ntttcp_server_stream(struct ntttcp_test_endpoint *endpoint);
