// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include "endpointsync.h"

#define SEC_TO_USEC			1000000
#define THROUGHPUT_INTERVAL_POLLS	1000    /* report throughput in every 1000 test status polls */
#define TEST_STATUS_POLL_INTERVAL_SEC	0.0005  /* each test status poll has 0.0005 sec */ 
#define TEST_STATUS_POLL_INTERVAL_U_SEC	TEST_STATUS_POLL_INTERVAL_SEC * SEC_TO_USEC  /* the 500 micro-sec */
#define THROUGHPUT_INTERVAL_SEC		TEST_STATUS_POLL_INTERVAL_SEC * THROUGHPUT_INTERVAL_POLLS /* 0.5 sec */
#define THROUGHPUT_INTERVAL_U_SEC	THROUGHPUT_INTERVAL_SEC * SEC_TO_USEC        /* 500,000 micro-sec */
#define CONN_CREATION_TIMEOUT_U_SEC	SEC_TO_USEC * 2

struct report_segment
{
	double		interval_sec;
	struct timeval	time;
	uint64_t	bytes;
};

void run_ntttcp_throughput_management(struct ntttcp_test_endpoint *tep);
