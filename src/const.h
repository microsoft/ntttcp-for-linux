// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define TOOL_NAME "NTTTCP for Linux"
#define TOOL_VERSION "1.3.0"
#define AUTHOR_NAME "Shihua (Simon) Xiao, sixiao@microsoft.com"

#define TCP 				SOCK_STREAM
#define UDP 				SOCK_DGRAM

#define ROLE_SENDER			1
#define ROLE_RECEIVER			2

#define TEST_UNKNOWN			10
#define TEST_NOT_STARTED		11
#define TEST_RUNNING			12
#define TEST_FINISHED			13
#define TEST_INTERRUPTED		14

/* max values */
#define MAX_CONNECTIONS_PER_THREAD	512
#define MAX_NUM_THREADS			512
#define MAX_REMOTE_ENDPOINTS		8
#define MAX_EPOLL_EVENTS		512
#define MAX_NUM_TOTAL_CONNECTIONS	MAX_NUM_THREADS * MAX_CONNECTIONS_PER_THREAD
/* Maximum size of sending a UDP packet is (64K - 1) - IP header - UDP header */
#define MAX_UDP_SEND_SIZE		(65535 - 8 - 20)

/* default values */
#define THREAD_STACK_SIZE		65536
#define DEFAULT_NUM_THREADS		16
#define DEFAULT_CONN_PER_THREAD		4
#define DEFAULT_BASE_DST_PORT		5001
#define DEFAULT_BASE_SRC_PORT 		25001
#define DEFAULT_RECV_BUFFER_SIZE	64 * 1024
#define DEFAULT_SEND_BUFFER_SIZE	128 * 1024
#define DEFAULT_TEST_DURATION		60

#define NO_ERROR			0
#define ERROR_GENERAL			-1000
#define ERROR_ARGS			-1001
#define ERROR_MEMORY_ALLOC		-1002
#define ERROR_PTHREAD_CREATE		-1003
#define ERROR_LISTEN			-1104
#define ERROR_ACCEPT			-1105
#define ERROR_SELECT			-1106
#define ERROR_EPOLL			-1107
#define ERROR_NETWORK_READ		-1108
#define ERROR_NETWORK_WRITE		-1109
#define ERROR_RECEIVER_NOT_READY	-1110

/* /proc file for re-transmit counters */
#define TCP_SECTION			"Tcp"
#define PROC_FILE_SNMP			"/proc/net/snmp"
#define PROC_FILE_NETSTAT 		"/proc/net/netstat"
#define CPU_SPEED_MHZ			"cpu MHz"
#define PROC_FILE_CPUINFO		"/proc/cpuinfo"

#define PROC_FILE_STAT			"/proc/stat"
