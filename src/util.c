// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "util.h"

void print_flags(struct ntttcp_test *test)
{
	if (test->server_role)
 		printf("%s\n", "*** receiver role");
	if (test->client_role)
		printf("%s\n", "*** sender role");
	if (test->daemon)
		printf("%s\n", "*** run as daemon");
	if (test->server_role && test->use_epoll )
		printf("%s\n", "*** use epoll()");

	if (test->no_synch )
		printf("%s\n", "*** no sender/receiver synch");

	//printf("%s:\t\t\t %s\n", "mapping", test->mapping);

	if (test->client_role)
		printf("%s:\t\t\t %d X %d\n", "threads", test->parallel, test->conn_per_thread);
	else
		printf("%s:\t\t\t %d\n", "threads", test->parallel);

	if (test->cpu_affinity == -1)
		printf("%s:\t\t\t %s\n", "cpu affinity", "*" );
	else
		printf("%s:\t\t\t %d\n", "cpu affinity", test->cpu_affinity );

	printf("%s:\t\t\t %s\n", "server address", test->bind_address);

	if (test->domain == AF_INET)
		printf("%s:\t\t\t\t %s\n", "domain", "IPv4");
	if (test->domain == AF_INET6)
		printf("%s:\t\t\t\t %s\n", "domain", "IPv6");
	if (test->protocol == TCP)
		printf("%s:\t\t\t %s\n", "protocol", "TCP");
	else if (test->protocol == UDP)
		printf("%s:\t\t\t %s\n", "protocol", "UDP");
	else
		printf("%s:\t\t\t %s\n", "protocol", "unsupported");

	printf("%s:\t %d\n", "server port starting at", test->server_base_port);

	if (test->server_role)
		printf("%s:\t %ld\n", "receiver socket buffer (bytes)", test->recv_buf_size);
	if (test->client_role)
		printf("%s:\t %ld\n", "sender socket buffer (bytes)", test->send_buf_size);

	printf("%s:\t\t %d\n", "test duration (sec)", test->duration);
	printf("%s:\t\t\t %s\n", "verbose mode", test->verbose ? "enabled" : "disabled");
	printf("---------------------------------------------------------\n");
}

void print_usage()
{
	printf("Author: %s\n", AUTHOR_NAME);
	printf("ntttcp: [-r|-s|-D|-m <mapping>|-n|-6|-u|-p|-b|-B|-t|-N|-V|-v|-h]\n\n");
	printf("\t-r   Run as a receiver\n");
	printf("\t-s   Run as a sender\n");
	printf("\t-D   Run as daemon\n");
	printf("\t-e   [receiver only] use epoll() instead of select()\n");

	printf("\t-P   Number of ports listening on receiver side\n");
	printf("\t-n   [sender only] number of connections per receiver port    [default: %d]  [max: %d]\n", DEFAULT_CONN_PER_THREAD, MAX_CONNECTIONS_PER_THREAD);

	printf("\t-6   IPv6 mode    [default: IPv4]\n");
	printf("\t-u   UDP mode     [default: TCP]\n");
	printf("\t-p   Port number, or starting port number    [default: %d]\n", DEFAULT_BASE_PORT);
	printf("\t-b   <recv buffer size>    [default: %d]\n", DEFAULT_RECV_BUFFER_SIZE);
	printf("\t-B   <send buffer size>    [default: %d]\n", DEFAULT_SEND_BUFFER_SIZE);
	printf("\t-t   Time of test duration in seconds    [default: %d]\n", DEFAULT_TEST_DURATION);
	printf("\t-N   No sync, senders will start sending as soon as possible.\n");

	printf("\t-V   Verbose mode\n");
	printf("\t-h   Help, tool usage\n");

	printf("\t-m   <mapping>\tfor the purpose of compatible with Windows ntttcp usage\n");
	printf("\t\t  Where a mapping is a NumberOfReceiverPorts,Processor,BindingIPAddress set:\n");
	printf("\t\t  NumberOfReceiverPorts:    [default: %d]  [max: %d]\n", DEFAULT_NUM_THREADS, MAX_NUM_THREADS);
	printf("\t\t  Processor:\t\t*, or cpuid such as 0, 1, etc \n");
	printf("\t\t  e.g. -m 8,*,192.168.1.1\n");
	printf("\t\t\t  If receiver role: 8 threads running on all processors;\n\t\t\tand listening on 8 ports of network on 192.168.1.1.\n");
	printf("\t\t\t  If sender role: receiver has 8 threads running and listening on 8 ports of network on 192.168.1.1;\n\t\t\tand all sender threads will run on all processors.\n");

	printf("Example:\n");
	printf("\treceiver:\n");
	printf("\t1) ./ntttcp -r\n");
	printf("\t2) ./ntttcp -r192.168.1.1\n");
	printf("\t3) ./ntttcp -r -m 8,*,192.168.1.1 -6\n");
	printf("\t4) ./ntttcp -r -m 8,0,192.168.1.1 -6 -V\n");
	printf("\tsender:\n");
	printf("\t1) ./ntttcp -s\n");
	printf("\t2) ./ntttcp -s192.168.1.1\n");
	printf("\t3) ./ntttcp -s -m 8,*,192.168.1.1 -n 16 -6\n");
	printf("\t4) ./ntttcp -s -m 8,0,192.168.1.1 -n 16 -6 -V\n");
}

void print_version()
{
	printf("%s %s\n", TOOL_NAME, TOOL_VERSION);
	printf("---------------------------------------------------------\n");
}

int process_mappings(struct ntttcp_test *test)
{
	int state = S_THREADS, threads = 0;
	char* token = NULL;
	int cpu = -1, total_cpus = 0;

	state = S_THREADS;
	char * element = strdup(test->mapping);

	while ((token = strsep(&element, ",")) != NULL)	{
		if (S_THREADS == state)		{
			threads = atoi(token);

			if (0 > threads) {
				return ERROR_ARGS;
			}
			test->parallel = threads;
			++state;
		}
		else if (S_PROCESSOR == state) {
			if (0 == strcmp(token, "*")){
				//do nothing
			}
			else{
				cpu = atoi(token);
				total_cpus = sysconf(_SC_NPROCESSORS_ONLN);
				if (total_cpus < 1) {
					PRINT_ERR("process_mappings: cannot set cpu affinity because we failed to determine number of CPUs online");
					continue;
				}

				if (cpu < -1 || cpu > total_cpus - 1) {
					PRINT_ERR("process_mappings: cpu specified is not in allowed scope");
					return ERROR_ARGS;
				}
				test->cpu_affinity = cpu;
			}
			++state;
		}
		else if (S_HOST == state) {
			test->bind_address = token;
			++state;
		}
		else
		{
			PRINT_ERR("process_mappings: unexpected parameters in mapping");
			return ERROR_ARGS;
		}
	}
	return NO_ERROR;
}

/* Check flag or role compatibility; set default value for some params */
int verify_args(struct ntttcp_test *test)
{
	bool verbose_log = test->verbose;

	if (test->server_role && test->client_role) {
		PRINT_ERR("both sender and receiver roles provided");
		return ERROR_ARGS;
	}

	if (!strcmp(test->mapping, "")) {
		PRINT_ERR("no mapping provided");
		return ERROR_ARGS;
	}

	if (test->domain == AF_INET6 && !strstr( test->bind_address, ":") ) {
		PRINT_ERR("invalid ipv6 address provided");
		return ERROR_ARGS;
	}

	if (test->domain == AF_INET && !strstr( test->bind_address, ".") ) {
		PRINT_ERR("invalid ipv4 address provided");
		return ERROR_ARGS;
	}

	if (!test->server_role && !test->client_role) {
		PRINT_INFO("no role specified. use receiver role");
		test->server_role = true;
	}

	if (test->conn_per_thread > MAX_CONNECTIONS_PER_THREAD) {
		PRINT_INFO("too many connections per server port. use the max value");
		test->conn_per_thread = MAX_CONNECTIONS_PER_THREAD;
	}

	if (test->parallel > MAX_NUM_THREADS) {
		PRINT_INFO("too many threads. use the max value");
		test->parallel = MAX_NUM_THREADS;
	}

	if (test->domain == AF_INET6 && strcmp( test->bind_address, "0.0.0.0")== 0 )
		test->bind_address = "::";

	if (test->client_role) {
		if (test->use_epoll)
			PRINT_DBG("ignore '-e' on sender role");

	}

	if (test->protocol == UDP && test->send_buf_size > MAX_UDP_SEND_SIZE) {
		PRINT_INFO("the UDP send size is too big. use the max value");
		test->send_buf_size = MAX_UDP_SEND_SIZE;
	}

	return NO_ERROR;
}

int parse_arguments(struct ntttcp_test *test, int argc, char **argv)
{
	static struct option longopts[] =
	{
		{"receiver", optional_argument, NULL, 'r'},
		{"sender", optional_argument, NULL, 's'},
		{"daemon", no_argument, NULL, 'D'},
		{"epoll", no_argument, NULL, 'e'},
		{"mapping", required_argument, NULL, 'm'},
		{"nports", required_argument, NULL, 'P'},
		{"nconn", required_argument, NULL, 'n'},
		{"ipv6", no_argument, NULL, '6'},
		{"udp", no_argument, NULL, 'u'},
		{"base-port", required_argument, NULL, 'p'},
		{"receiver-buffer", required_argument, NULL, 'b'},
		{"send-buffer", required_argument, NULL, 'B'},
		{"duration", required_argument, NULL, 't'},
		{"nosynch", no_argument, NULL, 'N'},
		{"verbose", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	int flag;

	while ((flag = getopt_long(argc, argv, "r::s::Dem:P:n:6up:b:B:t:NVh", longopts, NULL)) != -1) {
		switch (flag) {
		case 'r':
			test->server_role = true;
			if (optarg)
				test->bind_address = optarg;
			break;

		case 's':
			test->client_role = true;
			if (optarg)
				test->bind_address = optarg;
			break;

		case 'D':
			test->daemon = true;
			break;

		case 'e':
			test->use_epoll = true;
			break;

		case 'm':
			test->mapping = optarg;
			process_mappings(test);
			break;

		case 'P':
			test->parallel = atoi(optarg);
			break;

		case 'n':
			test->conn_per_thread = atoi(optarg);
			break;

		case '6':
			test->domain = AF_INET6;
			break;

		case 'u':
			test->protocol = UDP;
			break;

		case 'p':
			test->server_base_port = atoi(optarg);
			break;

		case 'b':
			test->recv_buf_size = unit_atod(optarg);
			break;

		case 'B':
			test->send_buf_size = unit_atod(optarg);
			break;

		case 't':
			test->duration = atoi(optarg);
			break;

		case 'N':
			test->no_synch = true;
			break;

		case 'V':
			test->verbose = true;
			break;

		case 'h':
		default:
			print_usage();
			exit(ERROR_ARGS);
		}
	}
	return NO_ERROR;
}

void get_cpu_usage(struct cpu_usage *cu)
{
	struct timeval time;
	struct rusage usage;

	gettimeofday(&time, NULL);
	cu->time = time.tv_sec * 1000000.0 + time.tv_usec;
	cu->clock = clock();
	getrusage(RUSAGE_SELF, &usage);
	cu->user_time = usage.ru_utime.tv_sec * 1000000.0 + usage.ru_utime.tv_usec;
	cu->system_time = usage.ru_stime.tv_sec * 1000000.0 + usage.ru_stime.tv_usec;
}

double get_time_diff(struct timeval *t1, struct timeval *t2)
{
	return fabs( (t1->tv_sec + (t1->tv_usec / 1000000.0)) - (t2->tv_sec + (t2->tv_usec / 1000000.0)) );
}

void print_thread_result(int tid, long total_bytes, double test_duration)
{
	char *log = NULL, *log_tmp = NULL;

	if (tid == -1) {
		PRINT_INFO("\tThread\tTime(s)\tThroughput");
		PRINT_INFO("\t======\t=======\t==========");
	}
	else{
		if (test_duration == 0)
			return;

		log_tmp = format_throughput(total_bytes, test_duration);
		asprintf(&log, "\t%d\t %.2f\t %s", tid, test_duration, log_tmp);
		free(log_tmp);
		PRINT_INFO_FREE(log);
	}
}

void print_total_result(long total_bytes,
			uint64_t cycle_diff,
			double test_duration,
			struct cpu_usage *init_cpu_usage,
			struct cpu_usage *final_cpu_usage )
{
	char *log = NULL, *log_tmp = NULL;
	double time_diff;

	if (test_duration == 0)
		return;

	PRINT_INFO("#####  Totals:  #####");
	asprintf(&log, "test duration\t:%.2f seconds", test_duration);
	PRINT_INFO_FREE(log);
	asprintf(&log, "total bytes\t:%ld", total_bytes);
	PRINT_INFO_FREE(log);

	log_tmp = format_throughput(total_bytes, test_duration);
	asprintf(&log, "\t throughput\t:%s", log_tmp);
	free(log_tmp);
	PRINT_INFO_FREE(log);

	time_diff = final_cpu_usage->time - init_cpu_usage->time;
	asprintf(&log, "total cpu time\t:%.2f%%",
		(((final_cpu_usage->clock - init_cpu_usage->clock) * 1000000.0 / CLOCKS_PER_SEC) / time_diff) * 100);
	PRINT_INFO_FREE(log);

	asprintf(&log, "\t user time\t:%.2f%%",
		((final_cpu_usage->user_time - init_cpu_usage->user_time) / time_diff) * 100);
	PRINT_INFO_FREE(log);

	asprintf(&log, "\t system time\t:%.2f%%",
		((final_cpu_usage->system_time - init_cpu_usage->system_time) / time_diff) * 100);
	PRINT_INFO_FREE(log);

	asprintf(&log, "\t cpu cycles\t:%ld", cycle_diff);
	PRINT_INFO_FREE(log);

	asprintf(&log, "cycles/byte\t:%.2f", total_bytes == 0? 0 : (double)cycle_diff/(double)total_bytes);
	PRINT_INFO_FREE(log);

	printf("---------------------------------------------------------\n");
}

const long KIBI = 1<<10;
const long MEBI = 1<<20;
const long GIBI = 1<<30;
double unit_atod(const char *s)
{
	double n;
	char suffix = '\0';

	sscanf(s, "%lf%c", &n, &suffix);
	switch (suffix) {
	case 'g': case 'G':
		n *= GIBI;
		break;
	case 'm': case 'M':
		n *= MEBI;
		break;
	case 'k': case 'K':
		n *= KIBI;
		break;
	default:
		break;
	}
	return n;
}

const char *unit_bps[] =
{
	"bps",
	"Kbps",
	"Mbps",
	"Gbps"
};

char *format_throughput(long bytes_transferred, double test_duration)
{
	double tmp = 0;
	int unit_idx = 0;
	char *throughput;

	tmp = bytes_transferred * 8.0 / test_duration;
	while (tmp > 1000 && unit_idx < 3) {
		tmp /= 1000.0;
		unit_idx++;
	}

	asprintf(&throughput, "%.2f%s", tmp, unit_bps[unit_idx]);
	return throughput;
}

char *retrive_ip_address_str(struct sockaddr_storage *ss, char *ip_str, size_t maxlen)
{
	switch(ss->ss_family) {
	case AF_INET:
		inet_ntop(AF_INET, &(((struct sockaddr_in *)ss)->sin_addr), ip_str, maxlen);
		break;

	case AF_INET6:
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)ss)->sin6_addr), ip_str, maxlen);
		break;

	default:
		break;
	}
	return ip_str;
}

char *retrive_ip4_address_str(struct sockaddr_in *ss, char *ip_str, size_t maxlen)
{
	inet_ntop(AF_INET, &(ss->sin_addr), ip_str, maxlen);
	return ip_str;
}

char *retrive_ip6_address_str(struct sockaddr_in6 *ss, char *ip_str, size_t maxlen)
{
	inet_ntop(AF_INET6, &(ss->sin6_addr), ip_str, maxlen);
	return ip_str;
}

int set_socket_non_blocking(int fd)
{
	int flags, rtn;
	flags = fcntl (fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	flags |= O_NONBLOCK;
	rtn = fcntl (fd, F_SETFL, flags);
	if (rtn == -1)
		return -1;

	return 0;
}
