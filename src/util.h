// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <getopt.h>
#include <time.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "const.h"
#include "logger.h"

/* maintain the test parameters accepted from user */
struct ntttcp_test
{
	bool    server_role;        /* '-s' for client */
	bool    client_role;        /* '-r' for server */
	bool    daemon;             /* '-D' for daemon mode */
	bool    use_epoll;          /* '-e' for epoll() to watch fd for events. receiver only */
	char    *mapping;           /* '-m' for connection(s),Processor,StartReceiver IP map set */
	uint    parallel;           /*    Parallel connections, from -m flag */
	int     cpu_affinity;       /*    CPU affinity, from -m flag */
	char    *bind_address;      /*    Socket binding address */
	uint    conn_per_thread;    /* '-n' for number of connections per thread. sender only */
	bool    no_synch;           /* '-N' to disable sender/receiver synch */

	int     domain;              /* default for AF_INET, or '-6' for AF_INET6 */
	int     protocol;            /* default for SOCK_STREAM for TCP, or '-u' for SOCK_DGRAM for UDP (does not support UDP for now) */
	uint    server_base_port;    /* '-p' for server listening base port */
	double  recv_buf_size;       /* '-b' for receive buffer option */
	double  send_buf_size;       /* '-B' for send buffer option */
	int     duration;            /* '-t' for total duration in sec of test */
	int     verbose;             /* '-V' for verbose logging */
};

/* manage the ntttcp sender/client, or receiver/server endpoints */
struct ntttcp_test_endpoint{
	int    endpoint_role;        /* sender or receiver role */
	struct ntttcp_test *test;
	int    state;                /* test state */
	int    confirmed_duration;   /* test duration exchanged and confirmed by both sides */
	struct timeval start_time;   /* timestamp of test started on this endpoint */
	struct timeval end_time;     /* timestamp of test ended on this endpoint */
	int    synch_thread;         /* the synch channel for sender/receiver sync */

	struct ntttcp_stream_client **client_streams; /* alloc memory for this if client/sender role */
	struct ntttcp_stream_server **server_streams; /* alloc memory for this if server/receiver role */

	pthread_t *data_threads;
};

struct ntttcp_stream_client{
	int     domain;
	int     protocol;
	char    *bind_address;
	uint    server_port;
	double  send_buf_size;
	int     is_sync_thread;
	bool    no_synch;
	int     verbose;
};

struct ntttcp_stream_server{
	int     domain;
	int     protocol;
	char    *bind_address;
	uint    server_port;
	double  recv_buf_size;
	int     is_sync_thread;
	bool    no_synch;
	int     verbose;
	bool    use_epoll;

	int     listener;     /* this is the socket to listen on port to accept new connections */
	int     max_fd;       /* track the max socket fd */
	fd_set  read_set;     /* set of read sockets */
	fd_set  write_set;    /* set of write sockets */
	int     state;
	long    total_bytes_transferred;
};

struct cpu_usage{
	clock_t   clock;
	double    time;
	double    user_time;
	double    system_time;
};


enum {S_THREADS = 0, S_PROCESSOR, S_HOST, S_DONE};

int parse_arguments(struct ntttcp_test *test, int argc, char **argv);
int process_mappings(struct ntttcp_test *test);
int verify_args(struct ntttcp_test *test);

void print_flags(struct ntttcp_test *test);
void print_usage();
void print_version();

struct ntttcp_test *new_ntttcp_test();
struct ntttcp_test_endpoint *new_ntttcp_test_endpoint(struct ntttcp_test *test, int endpoint_role);
void free_ntttcp_test_endpoint_and_test(struct ntttcp_test_endpoint* e);
struct ntttcp_stream_client *new_ntttcp_client_stream(struct ntttcp_test *test);
struct ntttcp_stream_server *new_ntttcp_server_stream(struct ntttcp_test *test);
void default_ntttcp_test(struct ntttcp_test *test);

void fill_buffer(register char *buf, register int count);
double unit_atod(const char *s);

void get_cpu_usage(struct cpu_usage *cu);
double get_time_diff(struct timeval *t1, struct timeval *t2);
void print_total_result(long total_bytes, uint64_t cycle_diff, double test_duration, struct cpu_usage *init_cpu_usage, struct cpu_usage *final_cpu_usage );
void print_thread_result(int tid, long total_bytes, double test_duration);
char *format_throughput(long bytes_transferred, double test_duration);
char *retrive_ip_address_str(struct sockaddr_storage *ss, char *ip_str, size_t maxlen);
