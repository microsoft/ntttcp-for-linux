// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "parameter.h"

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
		printf("%s:\t\t\t %d X %d X %d\n", "connections", test->server_ports, test->threads_per_server_port, test->conns_per_thread);
	else
		printf("%s:\t\t\t\t %d\n", "ports", test->server_ports);

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

	if (test->warmup == 0)
		printf("%s:\t\t %s\n", "test warm-up (sec)", "no");
	else
		printf("%s:\t\t %d\n", "test warm-up (sec)", test->warmup);

	if (test->duration == 0)
		printf("%s:\t\t %s\n", "test duration (sec)", "continuous");
	else
		printf("%s:\t\t %d\n", "test duration (sec)", test->duration);

	if (test->cooldown == 0)
		printf("%s:\t\t %s\n", "test cool-down (sec)", "no");
	else
		printf("%s:\t\t %d\n", "test cool-down (sec)", test->cooldown);

	printf("%s:\t %s\n", "show system tcp retransmit", test->show_tcp_retransmit ? "yes" : "no");

	if (test->save_xml_log)
		printf("%s:\t %s\n", "save output to xml file:", test->xml_log_filename);

	printf("%s:\t\t\t %s\n", "verbose mode", test->verbose ? "enabled" : "disabled");
	printf("---------------------------------------------------------\n");
}

void print_usage()
{
	printf("Author: %s\n", AUTHOR_NAME);
	printf("ntttcp: [-r|-s|-D|-M|-L|-e|-H|-P|-n|-l|-6|-u|-p|-f|-b|-W|-t|-C|-N|-R|-x|-V|-h|-m <mapping>\n\n");
	printf("\t-r   Run as a receiver\n");
	printf("\t-s   Run as a sender\n");
	printf("\t-D   Run as daemon\n");

	printf("\t-M   [receiver only] multi-clients mode\n");
	printf("\t-L   [sender only] indicates this is the last client when receiver is running with multi-clients mode\n");

	printf("\t-e   [receiver only] use epoll() instead of select()\n");
	printf("\t-H   [receiver only] hold receiver always running even after one test finished\n");

	printf("\t-P   Number of ports listening on receiver side	[default: %d] [max: %d]\n", DEFAULT_NUM_SERVER_PORTS, MAX_NUM_SERVER_PORTS);
	printf("\t-n   [sender only] number of threads per each receiver port     [default: %d] [max: %d]\n", DEFAULT_THREADS_PER_SERVER_PORT, MAX_THREADS_PER_SERVER_PORT);
	printf("\t-l   [sender only] number of connections per each sender thread [default: %d] [max: %d]\n", DEFAULT_CLIENT_CONNS_PER_THREAD, MAX_CLIENT_CONNS_PER_THREAD);

	printf("\t-6   IPv6 mode    [default: IPv4]\n");
	printf("\t-u   UDP mode     [default: TCP]\n");
	printf("\t-p   Destination port number, or starting port number    [default: %d]\n", DEFAULT_BASE_DST_PORT);
	printf("\t-f   Fixed source port number, or starting port number    [default: %d]\n", DEFAULT_BASE_SRC_PORT);
	printf("\t-b   <buffer size>    [default: %d (receiver); %d (sender)]\n", DEFAULT_RECV_BUFFER_SIZE, DEFAULT_SEND_BUFFER_SIZE);

	printf("\t-W   Warm-up time in seconds          [default: %d]\n", DEFAULT_WARMUP_SEC);
	printf("\t-t   Time of test duration in seconds [default: %d]\n", DEFAULT_TEST_DURATION);
	printf("\t-C   Cool-down time in seconds        [default: %d]\n", DEFAULT_COOLDOWN_SEC);
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
			test->threads_per_server_port = 1;
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

	if (test->server_ports > MAX_NUM_SERVER_PORTS) {
		PRINT_INFO("too many number-of-server-ports. use the max value");
		test->server_ports = MAX_NUM_SERVER_PORTS;
	}

	if (test->client_role && test->threads_per_server_port > MAX_THREADS_PER_SERVER_PORT) {
		PRINT_INFO("too many threads-per-server-port. use the max value");
		test->threads_per_server_port = MAX_THREADS_PER_SERVER_PORT;
	}

	if (test->client_role && test->conns_per_thread > MAX_CLIENT_CONNS_PER_THREAD) {
		PRINT_INFO("too many connections-per-thread. use the max value");
		test->conns_per_thread = MAX_CLIENT_CONNS_PER_THREAD;
	}

	if (test->server_ports < 1) {
		PRINT_INFO("invalid number-of-server-ports provided. use 1");
		test->server_ports = 1;
	}

	if (test->client_role && test->threads_per_server_port < 1) {
		PRINT_INFO("invalid threads-per-server-port provided. use 1");
		test->threads_per_server_port = 1;
	}

	if (test->client_role && test->conns_per_thread < 1) {
		PRINT_INFO("invalid connections-per-thread provided. use 1");
		test->conns_per_thread = 1;
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

	if (test->client_role) {
		if (test->client_base_port > 0 && test->client_base_port <= 1024) {
			test->client_base_port = DEFAULT_BASE_SRC_PORT;
			PRINT_DBG("source port is too small. use the default value");
		}

		if ((int)(MAX_LOCAL_IP_PORT - test->client_base_port)
		    < (int)(test->server_ports * test->threads_per_server_port * test->conns_per_thread)) {
			PRINT_ERR("source port is too high to provide a sufficient range of ports for your test");
		}
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

	if (test->warmup < 0) {
		test->warmup = DEFAULT_WARMUP_SEC;
		PRINT_INFO("invalid test warm-up seconds provided. use the default value");
	}

	if(test->cooldown <0) {
		test->cooldown = DEFAULT_COOLDOWN_SEC;
		PRINT_INFO("invalid test cool-down seconds provided. use the default value");
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
		{"nthread", required_argument, NULL, 'n'},
		{"nconnections", required_argument, NULL, 'l'},
		{"ipv6", no_argument, NULL, '6'},
		{"udp", no_argument, NULL, 'u'},
		{"base-dst-port", required_argument, NULL, 'p'},
		{"base-src-port", optional_argument, NULL, 'f'},
		{"buffer", required_argument, NULL, 'b'},
		{"warmup", required_argument, NULL, 'W'},
		{"duration", required_argument, NULL, 't'},
		{"cooldown", required_argument, NULL, 'C'},
		{"no-synch", no_argument, NULL, 'N'},
		{"show-retrans", no_argument, NULL, 'R'},
		{"save-xml", optional_argument, NULL, 'x'},
		{"verbose", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};
	*/

	int opt;

	while ((opt = getopt(argc, argv, "r::s::DMLeHm:P:n:l:6up:f::b:W:t:C:NRx::Vh")) != -1) {
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
			test->threads_per_server_port = atoi(optarg);
			break;

		case 'l':
			test->conns_per_thread = atoi(optarg);
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

		case 'W':
			test->warmup = atoi(optarg);
			break;

		case 't':
			test->duration = atoi(optarg);
			break;

		case 'C':
			test->cooldown = atoi(optarg);
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
