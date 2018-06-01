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
	if (test->server_role && test->use_epoll)
		printf("%s\n", "*** use epoll()");
	if (test->server_role && !test->exit_after_done)
		printf("%s\n", "*** hold receiver always running");

	if (test->multi_clients_mode)
		printf("%s:\t\t %s\n", "multiple clients", "yes");

	if (test->last_client)
		printf("%s:\t\t\t %s\n", "last client", "yes");

	if (test->no_synch)
		printf("%s\n", "*** no sender/receiver synch");

	//printf("%s:\t\t\t %s\n", "mapping", test->mapping);

	if (test->client_role)
		printf("%s:\t\t\t %d X %d\n", "threads", test->server_ports, test->conn_per_server_port);
	else
		printf("%s:\t\t\t %d\n", "threads", test->server_ports);

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

	if (test->client_role && test->client_base_port > 0)
		printf("%s:\t %d\n", "client source port starting at", test->client_base_port);

	if (test->server_role)
		printf("%s:\t %ld\n", "receiver socket buffer (bytes)", test->recv_buf_size);
	if (test->client_role)
		printf("%s:\t %ld\n", "sender socket buffer (bytes)", test->send_buf_size);

	if (test->duration == 0)
		printf("%s:\t\t %s\n", "test duration (sec)", "continuous");
	else
		printf("%s:\t\t %d\n", "test duration (sec)", test->duration);

	printf("%s:\t %s\n", "show system tcp retransmit", test->show_tcp_retransmit ? "yes" : "no");

	if (test->save_xml_log)
		printf("%s:\t %s\n", "save output to xml file:", test->xml_log_filename);

	printf("%s:\t\t\t %s\n", "verbose mode", test->verbose ? "enabled" : "disabled");
	printf("---------------------------------------------------------\n");
}

void print_usage()
{
	printf("Author: %s\n", AUTHOR_NAME);
	printf("ntttcp: [-r|-s|-D|-M|-L|-e|-H|-P|-n|-6|-u|-p|-f|-b|-t|-N|-R|-x|-V|-h|-m <mapping>\n\n");
	printf("\t-r   Run as a receiver\n");
	printf("\t-s   Run as a sender\n");
	printf("\t-D   Run as daemon\n");

	printf("\t-M   [receiver only] multi-clients mode\n");
	printf("\t-L   [sender only] indicates this is the last client when receiver is running with multi-clients mode\n");

	printf("\t-e   [receiver only] use epoll() instead of select()\n");
	printf("\t-H   [receiver only] hold receiver always running even after one test finished\n");

	printf("\t-P   Number of ports listening on receiver side\n");
	printf("\t-n   [sender only] number of connections(threads) per receiver port    [default: %d]  [max: %d]\n", DEFAULT_CONNS_PER_SERVER_PORT, MAX_CONNS_PER_SERVER_PORT);

	printf("\t-6   IPv6 mode    [default: IPv4]\n");
	printf("\t-u   UDP mode     [default: TCP]\n");
	printf("\t-p   Destination port number, or starting port number    [default: %d]\n", DEFAULT_BASE_DST_PORT);
	printf("\t-f   Fixed source port number, or starting port number    [default: %d]\n", DEFAULT_BASE_SRC_PORT);
	printf("\t-b   <buffer size>    [default: %d (receiver); %d (sender)]\n", DEFAULT_RECV_BUFFER_SIZE, DEFAULT_SEND_BUFFER_SIZE);

	printf("\t-t   Time of test duration in seconds    [default: %d]\n", DEFAULT_TEST_DURATION);
	printf("\t-N   No sync, senders will start sending as soon as possible\n");
	printf("\t     Otherwise, will use 'destination port - 1' as sync port	[default: %d]\n", DEFAULT_BASE_DST_PORT - 1);

	printf("\t-R   Show system TCP retransmit counters in log from /proc\n");
	printf("\t-x   Save output to XML file, by default saves to %s\n", DEFAULT_LOG_FILE_NAME);
	printf("\t-V   Verbose mode\n");
	printf("\t-h   Help, tool usage\n");

	printf("\t-m   <mapping>\tfor the purpose of compatible with Windows ntttcp usage\n");
	printf("\t     Where a mapping is a 3-tuple of NumberOfReceiverPorts, Processor, ReceiverAddress:\n");
	printf("\t     NumberOfReceiverPorts:    [default: %d]  [max: %d]\n", DEFAULT_NUM_SERVER_PORTS, MAX_NUM_SERVER_PORTS);
	printf("\t     Processor:\t\t*, or cpuid such as 0, 1, etc \n");
	printf("\t     e.g. -m 8,*,192.168.1.1\n");
	printf("\t\t    If for receiver role: 8 threads listening on 8 ports (one port per thread) on the network 192.168.1.1;\n\t\t\tand those threads will run on all processors.\n");
	printf("\t\t    If for sender role: receiver has 8 ports listening on the network 192.168.1.1;\n\t\t\tsender will create 8 threads to talk to all of those receiver ports\n\t\t\t(1 sender thread to one receiver port; this can be overridden by '-n');\n\t\t\tand all sender threads will run on all processors.\n");

	printf("Example:\n");
	printf("\treceiver:\n");
	printf("\t1) ./ntttcp -r\n");
	printf("\t2) ./ntttcp -r192.168.1.1\n");
	printf("\t3) ./ntttcp -r -m 8,*,192.168.1.1 -6\n");
	printf("\t4) ./ntttcp -r -m 8,0,192.168.1.1 -6 -R -V\n");
	printf("\tsender:\n");
	printf("\t1) ./ntttcp -s\n");
	printf("\t2) ./ntttcp -s192.168.1.1\n");
	printf("\t3) ./ntttcp -s -m 8,*,192.168.1.1 -n 16 -6\n");
	printf("\t4) ./ntttcp -s -m 8,0,192.168.1.1 -n 16 -f25001 -6 -V\n");
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

			if (1 > threads) {
				return ERROR_ARGS;
			}
			test->server_ports = threads;
			test->conn_per_server_port = 1;
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

	if (test->client_role && test->multi_clients_mode) {
		PRINT_ERR("multi-clients mode ('-M') is only for receiver role");
		return ERROR_ARGS;
	}

	if (test->server_role && test->last_client) {
		PRINT_ERR("last-client ('-L') is only for sender role");
		return ERROR_ARGS;
	}

	if (test->conn_per_server_port > MAX_CONNS_PER_SERVER_PORT) {
		PRINT_INFO("too many connections per server port. use the max value");
		test->conn_per_server_port = MAX_CONNS_PER_SERVER_PORT;
	}

	if (test->server_ports > MAX_NUM_SERVER_PORTS) {
		PRINT_INFO("too many threads. use the max value");
		test->server_ports = MAX_NUM_SERVER_PORTS;
	}

	if (test->conn_per_server_port < 1) {
		PRINT_INFO("invalid connections-per-server-port provided. use 1");
		test->conn_per_server_port = 1;
	}

	if (test->server_ports < 1) {
		PRINT_INFO("invalid number-of-server-ports provided. use 1");
		test->server_ports = 1;
	}

	if (test->domain == AF_INET6 && strcmp( test->bind_address, "0.0.0.0")== 0 )
		test->bind_address = "::";

	if (test->client_role) {
		if (test->use_epoll)
			PRINT_DBG("ignore '-e' on sender role");
		if (!test->exit_after_done)
			PRINT_DBG("ignore '-H' on sender role");
	}

	if (test->server_role && test->client_base_port > 0) {
		PRINT_DBG("ignore '-f' on receiver role");
	}

	if (test->client_role && test->client_base_port > 0 && test->client_base_port <= 1024) {
		test->client_base_port = DEFAULT_BASE_SRC_PORT;
		PRINT_DBG("source port is too small. use the default value");
	}

	if (test->protocol == UDP && test->send_buf_size > MAX_UDP_SEND_SIZE) {
		PRINT_INFO("the UDP send size is too big. use the max value");
		test->send_buf_size = MAX_UDP_SEND_SIZE;
	}

	if (test->duration < 0) {
		test->duration = DEFAULT_TEST_DURATION;
		PRINT_INFO("invalid test duration provided. use the default value");
	}
	if (test->duration == 0) {
		PRINT_INFO("running test in continuous mode. please monitor throughput by other tools");
	}

	return NO_ERROR;
}

int parse_arguments(struct ntttcp_test *test, int argc, char **argv)
{
	/* long options, deprecated */
	/*
	static struct option longopts[] =
	{
		{"receiver", optional_argument, NULL, 'r'},
		{"sender", optional_argument, NULL, 's'},
		{"daemon", no_argument, NULL, 'D'},
		{"multi-clients", no_argument, NULL, 'M'},
		{"last-client", no_argument, NULL, 'L'},
		{"epoll", no_argument, NULL, 'e'},
		{"hold", no_argument, NULL, 'H'},
		{"mapping", required_argument, NULL, 'm'},
		{"nports", required_argument, NULL, 'P'},
		{"nconn", required_argument, NULL, 'n'},
		{"ipv6", no_argument, NULL, '6'},
		{"udp", no_argument, NULL, 'u'},
		{"base-dst-port", required_argument, NULL, 'p'},
		{"base-src-port", optional_argument, NULL, 'f'},
		{"buffer", required_argument, NULL, 'b'},
		{"duration", required_argument, NULL, 't'},
		{"no-synch", no_argument, NULL, 'N'},
		{"show-retrans", no_argument, NULL, 'R'},
		{"save-xml", optional_argument, NULL, 'x'},
		{"verbose", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};
	*/

	int opt;

	while ((opt = getopt(argc, argv, "r::s::DMLeHm:P:n:6up:f::b:t:NRx::Vh")) != -1) {
		switch (opt) {
		case 'r':
		case 's':
			if (opt == 'r') {
				test->server_role = true;
			} else {
				test->client_role = true;
			}

			if (optarg) {
				test->bind_address = optarg;
			} else {
				if(optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0])
					test->bind_address = argv[optind++];
			}
			break;

		case 'D':
			test->daemon = true;
			break;

		case 'M':
			test->multi_clients_mode = true;
			break;

		case 'L':
			test->last_client = true;
			break;

		case 'e':
			test->use_epoll = true;
			break;

		case 'H':
			test->exit_after_done = false;
			break;

		case 'm':
			test->mapping = optarg;
			process_mappings(test);
			break;

		case 'P':
			test->server_ports = atoi(optarg);
			break;

		case 'n':
			test->conn_per_server_port = atoi(optarg);
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

		case 'f':
			if (optarg) {
				test->client_base_port = atoi(optarg);
			} else {
				if (optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0]) {
				        test->client_base_port = atoi(argv[optind++]);
				} else {
				        test->client_base_port = DEFAULT_BASE_SRC_PORT;
				}
			}
			break;

		case 'b':
			test->recv_buf_size = unit_atod(optarg);
			test->send_buf_size = unit_atod(optarg);
			break;

		case 't':
			test->duration = atoi(optarg);
			break;

		case 'N':
			test->no_synch = true;
			break;

		case 'R':
			test->show_tcp_retransmit = true;
			break;

		case 'x':
			test->save_xml_log = true;
			if (optarg){
				test->xml_log_filename = optarg;
			} else {
				if(optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0])
					test->xml_log_filename = argv[optind++];
			}
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

void get_cpu_usage_from_proc_stat(struct cpu_usage_from_proc_stat *cups)
{
	unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
	user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0;

	FILE* file = fopen(PROC_FILE_STAT, "r");
	if (file == NULL) {
		PRINT_ERR("Cannot open /proc/stat");
		return;
	}

	char buffer[256];
	int cpus = -1;
	do {
		cpus++;
		char *s = fgets(buffer, 255, file);
		/* We should not reach to the file end, because we only read lines starting with 'cpu' */
		if (s == NULL) {
			PRINT_ERR("Error when reading /proc/stat");
			return;
		}

		/* Assume the first line is stat for all CPUs. */
		if (cpus == 0) {
			sscanf(buffer,
			      "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
			      &user, &nice, &system, &idle, &iowait,
			      &irq, &softirq, &steal, &guest, &guest_nice);
		}
	} while(buffer[0] == 'c' && buffer[1] == 'p' && buffer[2] == 'u');

	fclose(file);

	cups->nproc = MAX(cpus - 1, 1);
	cups->user_time = user;
	cups->system_time = system;
	cups->idle_time = idle;
	cups->iowait_time = iowait;
	cups->softirq_time = softirq;

	cups->total_time = user + nice + system + idle + iowait + irq + softirq + steal;
}

double get_time_diff(struct timeval *t1, struct timeval *t2)
{
	return fabs( (t1->tv_sec + (t1->tv_usec / 1000000.0)) - (t2->tv_sec + (t2->tv_usec / 1000000.0)) );
}

const long KIBI = 1<<10;
const long MEBI = 1<<20;
const long GIBI = 1<<30;
const int BYTE_TO_BITS = 8;

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

int process_test_results(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	unsigned int i;
	double cpu_speed_mhz;
	double test_duration = tepr->actual_test_time;
	uint64_t total_bytes = tepr->total_bytes;
	long double cpu_ps_total_diff;

	if (test_duration == 0)
		return -1;

	/* calculate for per-thread counters */
	for (i=0; i<tep->total_threads; i++){
		if (tep->results->threads[i]->is_sync_thread == true)
			continue;

		tepr->threads[i]->KBps = tepr->threads[i]->total_bytes / KIBI / tepr->threads[i]->actual_test_time;
		tepr->threads[i]->MBps = tepr->threads[i]->KBps / KIBI;
		tepr->threads[i]->mbps = tepr->threads[i]->MBps * BYTE_TO_BITS;
	}

	/* calculate for overall counters */
	cpu_speed_mhz = read_value_from_proc(PROC_FILE_CPUINFO, CPU_SPEED_MHZ);
	tepr->cpu_speed_mhz = cpu_speed_mhz;

	tepr->retrans_segments_per_sec = (tepr->final_tcp_retrans->retrans_segs - tepr->init_tcp_retrans->retrans_segs) / test_duration;
	tepr->tcp_lost_retransmit_per_sec = (tepr->final_tcp_retrans->tcp_lost_retransmit - tepr->init_tcp_retrans->tcp_lost_retransmit) / test_duration;
	tepr->tcp_syn_retrans_per_sec = (tepr->final_tcp_retrans->tcp_syn_retrans - tepr->init_tcp_retrans->tcp_syn_retrans) / test_duration;
	tepr->tcp_fast_retrans_per_sec = (tepr->final_tcp_retrans->tcp_fast_retrans - tepr->init_tcp_retrans->tcp_fast_retrans) / test_duration;
	tepr->tcp_forward_retrans_per_sec = (tepr->final_tcp_retrans->tcp_forward_retrans - tepr->init_tcp_retrans->tcp_forward_retrans) / test_duration;
	tepr->tcp_slowStart_retrans_per_sec = (tepr->final_tcp_retrans->tcp_slowStart_retrans - tepr->init_tcp_retrans->tcp_slowStart_retrans) / test_duration;
	tepr->tcp_retrans_fail_per_sec = (tepr->final_tcp_retrans->tcp_retrans_fail - tepr->init_tcp_retrans->tcp_retrans_fail) / test_duration;

	cpu_ps_total_diff = tepr->final_cpu_ps->total_time - tepr->init_cpu_ps->total_time;
	tepr->cpu_ps_user_usage = (tepr->final_cpu_ps->user_time - tepr->init_cpu_ps->user_time) / cpu_ps_total_diff;
	tepr->cpu_ps_system_usage = (tepr->final_cpu_ps->system_time - tepr->init_cpu_ps->system_time) / cpu_ps_total_diff;
	tepr->cpu_ps_idle_usage = (tepr->final_cpu_ps->idle_time - tepr->init_cpu_ps->idle_time) / cpu_ps_total_diff;
	tepr->cpu_ps_iowait_usage = (tepr->final_cpu_ps->iowait_time - tepr->init_cpu_ps->iowait_time) / cpu_ps_total_diff;
	tepr->cpu_ps_softirq_usage = (tepr->final_cpu_ps->softirq_time - tepr->init_cpu_ps->softirq_time) / cpu_ps_total_diff;

	/* calculate for counters for xml log (compatiable with Windows ntttcp.exe) */
	tepr->total_bytes_MB = total_bytes / MEBI;
	tepr->throughput_MBps = tepr->total_bytes_MB / test_duration;
	tepr->throughput_mbps = tepr->throughput_MBps * BYTE_TO_BITS;
	tepr->cycles_per_byte = total_bytes == 0 ? 0 :
			cpu_speed_mhz * 1000 * 1000 * test_duration * (tepr->final_cpu_ps->nproc) * (1 - tepr->cpu_ps_idle_usage) / total_bytes;
	tepr->packets_sent = 0;
	tepr->packets_received = 0;
	tepr->packets_retransmitted = tepr->final_tcp_retrans->retrans_segs - tepr->init_tcp_retrans->retrans_segs;
	tepr->cpu_busy_percent = ((tepr->final_cpu_usage->clock - tepr->init_cpu_usage->clock) * 1000000.0 / CLOCKS_PER_SEC)
				 / (tepr->final_cpu_usage->time - tepr->init_cpu_usage->time);
	tepr->errors = 0;

	return 0;
}

void print_test_results(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	uint64_t total_bytes = tepr->total_bytes;
	double test_duration = tepr->actual_test_time;

	unsigned int i;
	char *log = NULL, *log_tmp = NULL;

	if (test_duration == 0)
		return;

	if (tep->test->verbose) {
		PRINT_INFO("\tThread\tTime(s)\tThroughput");
		PRINT_INFO("\t======\t=======\t==========");
		for (i=0; i<tep->total_threads; i++) {
			if (tep->results->threads[i]->is_sync_thread == true)
				continue;

			log_tmp = format_throughput(tepr->threads[i]->total_bytes,
							tepr->threads[i]->actual_test_time);
			ASPRINTF(&log, "\t%d\t %.2f\t %s", i, tepr->threads[i]->actual_test_time, log_tmp);
			free(log_tmp);
			PRINT_INFO_FREE(log);
		}
	}

	PRINT_INFO("#####  Totals:  #####");
	ASPRINTF(&log, "test duration\t:%.2f seconds", test_duration);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "total bytes\t:%" PRIu64, total_bytes);
	PRINT_INFO_FREE(log);

	log_tmp = format_throughput(total_bytes, test_duration);
	ASPRINTF(&log, "\t throughput\t:%s", log_tmp);
	free(log_tmp);
	PRINT_INFO_FREE(log);

	if (tep->test->show_tcp_retransmit) {
		PRINT_INFO("tcp retransmit:");
		ASPRINTF(&log, "\t retrans_segments/sec\t:%.2f", tepr->retrans_segments_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t lost_retrans/sec\t:%.2f", tepr->tcp_lost_retransmit_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t syn_retrans/sec\t:%.2f", tepr->tcp_syn_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t fast_retrans/sec\t:%.2f", tepr->tcp_fast_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t forward_retrans/sec\t:%.2f", tepr->tcp_forward_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t slowStart_retrans/sec\t:%.2f", tepr->tcp_slowStart_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t retrans_fail/sec\t:%.2f", tepr->tcp_retrans_fail_per_sec);
		PRINT_INFO_FREE(log);
	}

	if (tepr->final_cpu_ps->nproc == tepr->init_cpu_ps->nproc) {
		ASPRINTF(&log, "cpu cores\t:%d", tepr->final_cpu_ps->nproc);
		PRINT_INFO_FREE(log);
	} else {
		ASPRINTF(&log, "number of CPUs does not match: initial: %d; final: %d", tepr->init_cpu_ps->nproc, tepr->final_cpu_ps->nproc);
		PRINT_ERR_FREE(log);
	}

	ASPRINTF(&log, "\t cpu speed\t:%.3fMHz", tepr->cpu_speed_mhz);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t user\t\t:%.2f%%", tepr->cpu_ps_user_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t system\t\t:%.2f%%", tepr->cpu_ps_system_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t idle\t\t:%.2f%%", tepr->cpu_ps_idle_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t iowait\t\t:%.2f%%", tepr->cpu_ps_iowait_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t softirq\t:%.2f%%", tepr->cpu_ps_softirq_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t cycles/byte\t:%.2f", tepr->cycles_per_byte);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "cpu busy (all)\t:%.2f%%", tepr->cpu_busy_percent * 100);
	PRINT_INFO_FREE(log);

	printf("---------------------------------------------------------\n");
}

size_t execute_system_cmd_by_process(char *command, char *type, char *output)
{
	FILE *pfp;
	size_t count, len;

	pfp = popen(command, type);
	if (pfp == NULL) {
		PRINT_ERR("Error opening process to execute command");
		return 0;
	}

	count = getline(&output, &len, pfp);

	fclose(pfp);
	return count;
}

unsigned int escape_char_for_xml(char *in, char *out)
{
	unsigned int count = 0;
	size_t pos_in = 0, pos_out = 0;

	for(pos_in = 0; in[pos_in]; pos_in++) {
		count++;
		switch (in[pos_in]) {
		case '>':
			memcpy(out+pos_out, "&gt;", 4);
			pos_out = pos_out + 4;
			break;
		case '<':
			memcpy(out+pos_out, "&lt;", 4);
			pos_out = pos_out + 4;
			break;
		case '&':
			memcpy(out+pos_out, "&amp;", 5);
			pos_out = pos_out + 5;
			break;
		case '\'':
			memcpy(out+pos_out, "&apos;", 6);
			pos_out = pos_out + 6;
			break;
		case '\"':
			memcpy(out+pos_out, "&quot;", 6);
			pos_out = pos_out + 6;
			break;
		case '\n':
			break;
		default:
			count--;
			memcpy(out+pos_out, in+pos_in, 1);
			pos_out++;
		}
	}
	return count;
}

int write_result_into_log_file(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test *test = tep->test;
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	char str_temp1[256];
	char str_temp2[2048];
	size_t count = 0;
	unsigned int i;

	memset(str_temp1, '\0', sizeof(char) * 256);
	memset(str_temp2, '\0', sizeof(char) * 2048);

	FILE *logfile = fopen(test->xml_log_filename, "w");
	if (logfile == NULL) {
		PRINT_ERR("Error opening file to write log");
		return -1;
	}

	gethostname(str_temp1, 256);
	fprintf(logfile, "<ntttcp%s computername=\"%s\" version=\"5.33-linux\">\n", tep->endpoint_role == ROLE_RECEIVER ? "r" : "s", str_temp1);
	fprintf(logfile, "	<parameters>\n");
	fprintf(logfile, "		<send_socket_buff>%lu</send_socket_buff>\n", test->send_buf_size);
	fprintf(logfile, "		<recv_socket_buff>%lu</recv_socket_buff>\n", test->recv_buf_size);
	fprintf(logfile, "		<port>%d</port>\n", test->server_base_port);
	fprintf(logfile, "		<sync_port>%s</sync_port>\n", "False");
	fprintf(logfile, "		<no_sync>%s</no_sync>\n", "False");
	fprintf(logfile, "		<wait_timeout_milliseconds>%d</wait_timeout_milliseconds>\n", 0);
	fprintf(logfile, "		<async>%s</async>\n", "False");
	fprintf(logfile, "		<verbose>%s</verbose>\n", test->verbose ? "True":"False");
	fprintf(logfile, "		<wsa>%s</wsa>\n", "False");
	fprintf(logfile, "		<use_ipv6>%s</use_ipv6>\n", test->domain == AF_INET6 ? "True":"False");
	fprintf(logfile, "		<udp>%s</udp>\n", test->protocol == UDP? "True":"False");
	fprintf(logfile, "		<verify_data>%s</verify_data>\n", "False");
	fprintf(logfile, "		<wait_all>%s</wait_all>\n", "False");
	fprintf(logfile, "		<run_time>%d</run_time>\n", test->duration);
	fprintf(logfile, "		<warmup_time>%d</warmup_time>\n", 0);
	fprintf(logfile, "		<cooldown_time>%d</cooldown_time>\n", 0);
	fprintf(logfile, "		<dash_n_timeout>%d</dash_n_timeout>\n", 0);
	fprintf(logfile, "		<bind_sender>%s</bind_sender>\n", "False");
	fprintf(logfile, "		<sender_name>%s</sender_name>\n", "NA");
	fprintf(logfile, "		<max_active_threads>%d</max_active_threads>\n", 0);
	fprintf(logfile, "		<tp>%s</tp>\n", "False");
	fprintf(logfile, "		<no_stdio_buffer>%s</no_stdio_buffer>\n", "False");
	fprintf(logfile, "		<throughput_Bpms>%d</throughput_Bpms>\n", 0);
	fprintf(logfile, "		<cpu_burn>%d</cpu_burn>\n", 0);
	fprintf(logfile, "		<latency_measurement>%s</latency_measurement>\n", "False");
	fprintf(logfile, "		<use_io_compl_ports>%s</use_io_compl_ports>\n", "NA");
	fprintf(logfile, "		<cpu_from_idle_flag>%s</cpu_from_idle_flag>\n", "False");
	fprintf(logfile, "		<get_estats>%s</get_estats>\n", "False");
	fprintf(logfile, "		<qos_flag>%s</qos_flag>\n", "False");
	fprintf(logfile, "		<jitter_measurement>%s</jitter_measurement>\n", "False");
	fprintf(logfile, "		<packet_spacing>%d</packet_spacing>\n", 0);
	fprintf(logfile, "	</parameters>\n");

	for(i = 0; i < tep->total_threads; i++ ){
		if (tep->results->threads[i]->is_sync_thread == true)
			continue;

		fprintf(logfile, "	<thread index=\"%i\">\n", i);
		fprintf(logfile, "		<realtime metric=\"s\">%.3f</realtime>\n", tepr->threads[i]->actual_test_time);
		fprintf(logfile, "		<throughput metric=\"KB/s\">%.3f</throughput>\n", tepr->threads[i]->KBps);
		fprintf(logfile, "		<throughput metric=\"MB/s\">%.3f</throughput>\n", tepr->threads[i]->MBps);
		fprintf(logfile, "		<throughput metric=\"mbps\">%.3f</throughput>\n", tepr->threads[i]->mbps);
		fprintf(logfile, "		<avg_bytes_per_compl metric=\"B\">%.3f</avg_bytes_per_compl>\n", 0.000);
		fprintf(logfile, "	</thread>\n");
	}

	fprintf(logfile, "	<total_bytes metric=\"MB\">%.6f</total_bytes>\n", tepr->total_bytes_MB);
	fprintf(logfile, "	<realtime metric=\"s\">%.6f</realtime>\n", tepr->actual_test_time);
	fprintf(logfile, "	<avg_bytes_per_compl metric=\"B\">%.3f</avg_bytes_per_compl>\n", 0.000);
	fprintf(logfile, "	<threads_avg_bytes_per_compl metric=\"B\">%.3f</threads_avg_bytes_per_compl>\n", 0.000);
	fprintf(logfile, "	<avg_frame_size metric=\"B\">%.3f</avg_frame_size>\n", 0.000);
	fprintf(logfile, "	<throughput metric=\"MB/s\">%.3f</throughput>\n", tepr->throughput_MBps);
	fprintf(logfile, "	<throughput metric=\"mbps\">%.3f</throughput>\n", tepr->throughput_mbps);
	fprintf(logfile, "	<total_buffers>%.3f</total_buffers>\n", 0.000);
	fprintf(logfile, "	<throughput metric=\"buffers/s\">%.3f</throughput>\n", 0.000);
	fprintf(logfile, "	<avg_packets_per_interrupt metric=\"packets/interrupt\">%.3f</avg_packets_per_interrupt>\n", 0.000);
	fprintf(logfile, "	<interrupts metric=\"count/sec\">%.3f</interrupts>\n", 0.000);
	fprintf(logfile, "	<dpcs metric=\"count/sec\">%.3f</dpcs>\n", 0.000);
	fprintf(logfile, "	<avg_packets_per_dpc metric=\"packets/dpc\">%.3f</avg_packets_per_dpc>\n", 0.000);
	fprintf(logfile, "	<cycles metric=\"cycles/byte\">%.3f</cycles>\n", tepr->cycles_per_byte);
	fprintf(logfile, "	<packets_sent>%lu</packets_sent>\n", tepr->packets_sent);
	fprintf(logfile, "	<packets_received>%lu</packets_received>\n", tepr->packets_received);
	fprintf(logfile, "	<packets_retransmitted>%lu</packets_retransmitted>\n", tepr->packets_retransmitted);
	fprintf(logfile, "	<errors>%d</errors>\n", tepr->errors);
	fprintf(logfile, "	<cpu metric=\"%%\">%.3f</cpu>\n", tepr->cpu_busy_percent * 100);
	fprintf(logfile, "	<bufferCount>%u</bufferCount>\n", 0);
	fprintf(logfile, "	<bufferLen>%u</bufferLen>\n", 0);
	fprintf(logfile, "	<io>%u</io>\n", 0);

	count = execute_system_cmd_by_process("uname -a", "r", str_temp1);
	escape_char_for_xml(str_temp1, str_temp2);
	fprintf(logfile, "	<os>%s</os>\n", count == 0 ? "Unknown" : str_temp2);

	fprintf(logfile, "</ntttcp%s>\n", tep->endpoint_role == ROLE_RECEIVER ? "r": "s");

	fclose(logfile);
	return 0;
}

char *format_throughput(uint64_t bytes_transferred, double test_duration)
{
	double tmp = 0;
	int unit_idx = 0;
	char *throughput;

	tmp = bytes_transferred * 8.0 / test_duration;
	while (tmp > 1000 && unit_idx < 3) {
		tmp /= 1000.0;
		unit_idx++;
	}

	ASPRINTF(&throughput, "%.2f%s", tmp, unit_bps[unit_idx]);
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
	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	flags |= O_NONBLOCK;
	rtn = fcntl(fd, F_SETFL, flags);
	if (rtn == -1)
		return -1;

	return 0;
}

uint64_t read_counter_from_proc(char *file_name, char *section, char *key)
{
	char *log;
	FILE *stream;
	char *line = NULL, *pch = NULL;
	size_t len = 0;
	ssize_t read;
	int key_found = 0;

	stream = fopen(file_name, "r");
	if (!stream) {
		ASPRINTF(&log, "failed to open file: %s. errno = %d", file_name, errno);
		PRINT_ERR_FREE(log);
		return 0;
	}

	/*
	 * example:
	 *
	 * Tcp: ... OutSegs RetransSegs InErrs OutRsts InCsumErrors
	 * Tcp: ... 8584582 27 0 5 0
	 *
	 * the first line contains the key;
	 * if a key is found, then,
	 * read the corresponding cell as value from next line
	 */
	while ((read = getline(&line, &len, stream)) != -1) {
		/* key is found, then read the value here */
		if (key_found >0) {
			pch = line;
			while ((pch = strtok(pch, " "))) {
				key_found--;
				if (key_found == 0)
					goto found;
				pch = NULL;
			}
		}
		/* try to locate the key */
		if (strcmp(section, line) < 0) {
			if (strstr(line, key) != NULL) {
				pch = line;
				while ((pch = strtok(pch, " "))) {
					key_found++;
					if (strcmp(pch, key) == 0)
						break;
					pch = NULL;
				}
			}
		}
	}

found:
	free(line);
	fclose(stream);

	return pch ? strtoull(pch, NULL, 10) : 0;
}

void get_tcp_retrans(struct tcp_retrans *tr)
{
	tr->retrans_segs		= read_counter_from_proc(PROC_FILE_SNMP,    TCP_SECTION, "RetransSegs");
	tr->tcp_lost_retransmit		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPLostRetransmit");
	tr->tcp_syn_retrans		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPSynRetrans");
	tr->tcp_fast_retrans		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPFastRetrans");
	tr->tcp_forward_retrans		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPForwardRetrans");
	tr->tcp_slowStart_retrans	= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPSlowStartRetrans");
	tr->tcp_retrans_fail		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPRetransFail");
}

double read_value_from_proc(char *file_name, char *key)
{
	char *log;
	FILE *stream;
	char *line = NULL, *pch = NULL;
	size_t len = 0;
	ssize_t read;
	double speed = 0;

	stream = fopen(file_name, "r");
	if (!stream) {
		ASPRINTF(&log, "failed to open file: %s. errno = %d", file_name, errno);
		PRINT_ERR_FREE(log);
		return 0;
	}

	/*
	 * example:
	 * ...
	 * cpu MHz         : 2394.462
	 * ...
	 */
	while ((read = getline(&line, &len, stream)) != -1) {
		if (strstr(line, key) != NULL) {
			pch = strstr(line, ":") + 1;
			break;
		}
	}
	speed = pch ? strtod(pch, NULL) : 0;
	if (speed == 0)
		PRINT_ERR("Failed to read CPU speed from /proc/cpuinfo");

	free(line);
	fclose(stream);

	return speed;
}

bool check_resource_limit(struct ntttcp_test *test)
{
	char *log;
	unsigned long soft_limit = 0;
	unsigned long hard_limit = 0;
	uint total_connections = 0;
	bool verbose_log = test->verbose;

	struct rlimit limitstruct;
	if(-1 == getrlimit(RLIMIT_NOFILE, &limitstruct))
		PRINT_ERR("Failed to load resource limits");

	soft_limit = (unsigned long)limitstruct.rlim_cur;
	hard_limit = (unsigned long)limitstruct.rlim_max;

	ASPRINTF(&log, "user limits for maximum number of open files: soft: %ld; hard: %ld",
			soft_limit,
			hard_limit);
	PRINT_DBG_FREE(log);

	if (test->client_role == true) {
		total_connections = test->server_ports * test->conn_per_server_port;
	} else {
		/*
		 * for receiver, just do a minial check;
		 * because we don't know how many conn_per_server_port will be used by sender.
		 */
		total_connections = test->server_ports * 1;
	}

	if (total_connections > soft_limit) {
		ASPRINTF(&log, "soft limit is too small: limit is %ld; but total connections will be %d X n",
				soft_limit,
				test->server_ports);
		PRINT_ERR_FREE(log);

		return false;
	} else {
		return true;
	}
}
