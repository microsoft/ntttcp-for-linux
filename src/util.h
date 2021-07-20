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
#include <time.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "ntttcp.h"
#include "oscounter.h"

struct ntttcp_test_endpoint_thread_result{
	bool	is_sync_thread;
	/* raw data of counters collected before and after test run */
	uint64_t	total_bytes;
	double	actual_test_time;

	/* fields can be calculated with above raw data */
	double	KBps;
	double	MBps;
	double	mbps;
};

struct ntttcp_test_endpoint_results{
	struct  ntttcp_test_endpoint *endpoint;

	/* raw data of counters collected before and after test run */
	uint64_t	total_bytes;
	double	actual_test_time;
	struct 	cpu_usage *init_cpu_usage;
	struct 	cpu_usage *final_cpu_usage;
	struct 	cpu_usage_from_proc_stat *init_cpu_ps;
	struct 	cpu_usage_from_proc_stat *final_cpu_ps;
	struct 	tcp_retrans *init_tcp_retrans;
	struct 	tcp_retrans *final_tcp_retrans;
	uint64_t	init_tx_packets;
	uint64_t	init_rx_packets;
	uint64_t	final_tx_packets;
	uint64_t	final_rx_packets;
	uint64_t	init_interrupts;
	uint64_t	final_interrupts;

	/* point to per-thread result */
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
	uint64_t	total_interrupts;
	double	packets_per_interrupt;

	/* fields for xml log (compatible with Windows ntttcp.exe) */
	double	total_bytes_MB;
	double	throughput_MBps;
	double	throughput_mbps;
	double	cycles_per_byte;
	uint64_t	packets_sent;
	uint64_t	packets_received;
	uint64_t	packets_retransmitted;
	double		cpu_busy_percent;
	unsigned int	errors;
	unsigned int	average_rtt;
};

void fill_buffer(register char *buf, register int count);
double unit_atod(const char *s);
double get_time_diff(struct timeval *t1, struct timeval *t2);

int process_test_results(struct ntttcp_test_endpoint *tep);
void print_test_results(struct ntttcp_test_endpoint *tep);
int write_result_into_log_file(struct ntttcp_test_endpoint *tep);
int write_result_into_json_file(struct ntttcp_test_endpoint *tep);

char *format_throughput(uint64_t bytes_transferred, double test_duration);
char *retrive_ip_address_str(struct sockaddr_storage *ss, char *ip_str, size_t maxlen);
char *retrive_ip4_address_str(struct sockaddr_in *ss, char *ip_str, size_t maxlen);
char *retrive_ip6_address_str(struct sockaddr_in6 *ss, char *ip_str, size_t maxlen);
int set_socket_non_blocking(int fd);

void enable_fq_rate_limit(struct ntttcp_stream_client *sc, int sockfd);
bool check_resource_limit(struct ntttcp_test *test);
bool check_is_ip_addr_valid_local(int ss_family, char *ip_to_check);
