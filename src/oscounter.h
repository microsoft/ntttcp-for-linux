// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include "const.h"
#include "logger.h"

struct cpu_usage {
	clock_t clock;
	double time;
	double user_time;
	double system_time;
};

struct cpu_usage_from_proc_stat {
	unsigned int nproc;
	long long unsigned total_time;
	long long unsigned user_time;
	long long unsigned system_time;
	long long unsigned idle_time;
	long long unsigned softirq_time;
};

struct tcp_retrans {
	uint64_t retrans_segs;
	uint64_t tcp_lost_retransmit;
	uint64_t tcp_syn_retrans;
	uint64_t tcp_fast_retrans;
	uint64_t tcp_forward_retrans;
	uint64_t tcp_slowStart_retrans;
	uint64_t tcp_retrans_fail;
};

void get_cpu_usage(struct cpu_usage *cu);
void get_cpu_usage_from_proc_stat(struct cpu_usage_from_proc_stat *cups);
uint64_t get_interrupts_from_proc_by_dev(char *dev_name);
uint64_t get_single_value_from_os_file(char *if_name, char *tx_or_rx);

double read_value_from_proc(char *file_name, char *key);
uint64_t read_counter_from_proc(char *file_name, char *section, char *key);
void get_tcp_retrans(struct tcp_retrans *tr);

#define MAX(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })
