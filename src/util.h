// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define __STDC_FORMAT_MACROS
#pragma once

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
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
#include "ntttcp.h"

struct cpu_usage{
	clock_t   clock;
	double    time;
	double    user_time;
	double    system_time;
};

struct cpu_usage_from_proc_stat{
	unsigned int nproc;
	long long unsigned total_time;
	long long unsigned user_time;
	long long unsigned system_time;
	long long unsigned idle_time;
	long long unsigned iowait_time;
	long long unsigned softirq_time;
};

struct tcp_retrans{
	uint64_t  retrans_segs;
	uint64_t  tcp_lost_retransmit;
	uint64_t  tcp_syn_retrans;
	uint64_t  tcp_fast_retrans;
	uint64_t  tcp_forward_retrans;
	uint64_t  tcp_slowStart_retrans;
	uint64_t  tcp_retrans_fail;
};

struct ntttcp_test_endpoint_thread_result{
	int		is_sync_thread;
	/* raw data of counters collected before and after test run */
	uint64_t	total_bytes;
	double		actual_test_time;

	/* fields can be calculated with above raw data */
	double		KBps;
	double		MBps;
	double		mbps;
};

struct ntttcp_test_endpoint_results{
	/* raw data of counters collected before and after test run */
	uint64_t	total_bytes;
	double	actual_test_time;
	struct 	cpu_usage *init_cpu_usage;
	struct 	cpu_usage *final_cpu_usage;
	struct 	cpu_usage_from_proc_stat *init_cpu_ps;
	struct 	cpu_usage_from_proc_stat *final_cpu_ps;
	struct 	tcp_retrans *init_tcp_retrans;
	struct 	tcp_retrans *final_tcp_retrans;

	/*point to per-thread result*/
	struct	ntttcp_test_endpoint_thread_result	**threads;

	/* fields can be calculated with above raw data or read from system */
	double	cpu_speed_mhz;
	double	time_diff;
	double	retrans_segments_per_sec;
	double	tcp_lost_retransmit_per_sec;
	double	tcp_syn_retrans_per_sec;
	double	tcp_fast_retrans_per_sec;
	double	tcp_forward_retrans_per_sec;
	double	tcp_slowStart_retrans_per_sec;
	double	tcp_retrans_fail_per_sec;
	double	cpu_ps_user_usage;
	double	cpu_ps_system_usage;
	double	cpu_ps_idle_usage;
	double	cpu_ps_iowait_usage;
	double	cpu_ps_softirq_usage;

	/* fields for xml log (compatiable with Windows ntttcp.exe) */
	double	total_bytes_MB;
	double	throughput_MBps;
	double	throughput_mbps;
	double	cycles_per_byte;
	uint64_t	packets_sent;
	uint64_t	packets_received;
	uint64_t	packets_retransmitted;
	double		cpu_busy_percent;
	unsigned int	errors;
};

enum {S_THREADS = 0, S_PROCESSOR, S_HOST, S_DONE};

int parse_arguments(struct ntttcp_test *test, int argc, char **argv);
int process_mappings(struct ntttcp_test *test);
int verify_args(struct ntttcp_test *test);

void print_flags(struct ntttcp_test *test);
void print_usage();
void print_version();

void fill_buffer(register char *buf, register int count);
double unit_atod(const char *s);

void get_cpu_usage(struct cpu_usage *cu);
void get_cpu_usage_from_proc_stat(struct cpu_usage_from_proc_stat *cups);
double get_time_diff(struct timeval *t1, struct timeval *t2);

int process_test_results(struct ntttcp_test_endpoint *tep);
void print_test_results(struct ntttcp_test_endpoint *tep);
int write_result_into_log_file(struct ntttcp_test_endpoint *tep);

char *format_throughput(uint64_t bytes_transferred, double test_duration);
char *retrive_ip_address_str(struct sockaddr_storage *ss, char *ip_str, size_t maxlen);
char *retrive_ip4_address_str(struct sockaddr_in *ss, char *ip_str, size_t maxlen);
char *retrive_ip6_address_str(struct sockaddr_in6 *ss, char *ip_str, size_t maxlen);
int set_socket_non_blocking(int fd);

double read_value_from_proc(char *file_name, char *key);
uint64_t read_counter_from_proc(char *file_name, char *section, char *key);
void get_tcp_retrans(struct tcp_retrans *tr);
bool check_resource_limit(struct ntttcp_test *test);

#define MAX(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })
