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
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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
double get_time_diff(struct timeval *t1, struct timeval *t2);
void print_total_result(uint64_t total_bytes, uint64_t cycle_diff, double test_duration, struct cpu_usage *init_cpu_usage, struct cpu_usage *final_cpu_usage );
void print_thread_result(int tid, uint64_t total_bytes, double test_duration);
char *format_throughput(uint64_t bytes_transferred, double test_duration);
char *retrive_ip_address_str(struct sockaddr_storage *ss, char *ip_str, size_t maxlen);
char *retrive_ip4_address_str(struct sockaddr_in *ss, char *ip_str, size_t maxlen);
char *retrive_ip6_address_str(struct sockaddr_in6 *ss, char *ip_str, size_t maxlen);
int set_socket_non_blocking(int fd);
