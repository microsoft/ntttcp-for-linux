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

	// printf("%s:\t\t\t %s\n", "mapping", test->mapping);

	if (test->client_role)
		printf("%s:\t\t\t %d X %d X %d\n",
			"connections",
			test->server_ports, test->threads_per_server_port, test->conns_per_thread);
	else
		printf("%s:\t\t\t\t %d\n", "ports", test->server_ports);

	if (test->cpu_affinity == -1)
		printf("%s:\t\t\t %s\n", "cpu affinity", "*");
	else
		printf("%s:\t\t\t %d\n", "cpu affinity", test->cpu_affinity);

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

	if (test->client_role && test->bandwidth_limit != 0)
		printf("%s:\t %ld\n", "bandwidth limit (bits/sec)", test->bandwidth_limit);

	if (test->client_role && test->fq_rate_limit != 0)
		printf("%s:\t %ld\n", "fq rate limit (bits/sec)", test->fq_rate_limit);

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
	if (strcmp(test->show_interface_packets, ""))
		printf("%s:\t\t %s\n", "show packets for", test->show_interface_packets);
	if (strcmp(test->show_dev_interrupts, ""))
		printf("%s:\t %s\n", "show device interrupts for", test->show_dev_interrupts);

	if (test->save_console_log)
		printf("%s:\t %s\n", "capture console output to:", test->console_log_filename);
	if (test->save_xml_log)
		printf("%s:\t %s\n", "save output to xml file:", test->xml_log_filename);
	if (test->save_json_log)
		printf("%s:\t %s\n", "save output to json file:", test->json_log_filename);

	printf("%s:\t\t\t %s\n", "quiet mode", test->quiet ? "enabled" : "disabled");
	printf("%s:\t\t\t %s\n", "verbose mode", test->verbose ? "enabled" : "disabled");
	printf("---------------------------------------------------------\n");
}

void print_usage()
{
	printf("Author: %s\n", AUTHOR_NAME);
	printf("ntttcp: [-r|-s|-D|-M|-L|-e|-H|-P|-n|-l|-6|-u|-p|-f|-b|-B|-W|-t|-C|-N|-O|-x|-j|-Q|-V|-h|-m <mapping>]\n");
	printf("        [--show-tcp-retrans|--show-nic-packets|--show-dev-interrupts|--fq-rate-limit]\n\n");
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
	printf("\t-b   <buffer size in n[KMG] Bytes>    [default: %d (receiver); %d (sender)]\n", DEFAULT_RECV_BUFFER_SIZE, DEFAULT_SEND_BUFFER_SIZE);
	printf("\t-B   <bandwidth limit in n[KMG] bits/sec> set limit to the bandwidth\n");

	printf("\t-W   Warm-up time in seconds          [default: %d]\n", DEFAULT_WARMUP_SEC);
	printf("\t-t   Time of test duration in seconds [default: %d]\n", DEFAULT_TEST_DURATION);
	printf("\t-C   Cool-down time in seconds        [default: %d]\n", DEFAULT_COOLDOWN_SEC);
	printf("\t-N   No sync, senders will start sending as soon as possible\n");
	printf("\t     Otherwise, will use 'destination port - 1' as sync port	[default: %d]\n", DEFAULT_BASE_DST_PORT - 1);
	printf("\t-O   Save console log to file, by default saves to %s\n", DEFAULT_CONSOLE_LOG_FILE_NAME);
	printf("\t-x   Save output to XML file, by default saves to %s\n", DEFAULT_XML_LOG_FILE_NAME);
	printf("\t-j   Save output to JSON file, by default saves to %s\n", DEFAULT_JSON_LOG_FILE_NAME);
	printf("\t-Q   Quiet mode\n");
	printf("\t-V   Verbose mode\n");
	printf("\t-h   Help, tool usage\n");

	printf("\t-m   <mapping>\tfor the purpose of compatible with Windows ntttcp usage\n");
	printf("\t     Where a mapping is a 3-tuple of NumberOfReceiverPorts, Processor, ReceiverAddress:\n");
	printf("\t     NumberOfReceiverPorts:    [default: %d]  [max: %d]\n", DEFAULT_NUM_SERVER_PORTS, MAX_NUM_SERVER_PORTS);
	printf("\t     Processor:\t\t*, or cpuid such as 0, 1, etc \n");
	printf("\t     e.g. -m 8,*,192.168.1.1\n");
	printf("\t\t    If for receiver role: 8 threads listening on 8 ports (one port per thread) on the network "
			"192.168.1.1;\n\t\t\tand those threads will run on all processors.\n");
	printf("\t\t    If for sender role: receiver has 8 ports listening on the network 192.168.1.1;\n\t\t\tsender will "
			"create 8 threads to talk to all of those receiver ports\n\t\t\t(1 sender thread to one receiver port; this "
			"can be overridden by '-n');\n\t\t\tand all sender threads will run on all processors.\n");
	printf("\n");

	printf("\t--show-tcp-retrans\tShow system TCP retransmit counters in log from /proc\n ");
	printf("\t--show-nic-packets <network interface name>\n");
	printf("\t\t\t\tShow number of packets transferred (tx and rx) through this network interface\n");
	printf("\t--show-dev-interrupts <device differentiator>\n");
	printf("\t\t\t\tShow number of interrupts for the devices specified by the differentiator\n");
	printf("\t\t\t\tExamples for differentiator: Hyper-V PCIe MSI, mlx4, Hypervisor callback interrupts, ...\n");
	printf("\t--fq-rate-limit\t\tLimit socket rate by Fair Queue (FQ) traffic policing\n");
	printf("\n");

	printf("Example:\n");
	printf("\treceiver:\n");
	printf("\t1) ./ntttcp -r\n");
	printf("\t2) ./ntttcp -r 192.168.1.1\n");
	printf("\t3) ./ntttcp -r -m 8,*,192.168.1.1 -6\n");
	printf("\t4) ./ntttcp -r -m 8,0,192.168.1.1 -6 --show-tcp-retrans --show-nic-packets eth0 --show-dev-interrupts mlx4 -V\n");
	printf("\tsender:\n");
	printf("\t1) ./ntttcp -s\n");
	printf("\t2) ./ntttcp -s 192.168.1.1\n");
	printf("\t3) ./ntttcp -s -m 8,*,192.168.1.1 -n 16 -6\n");
	printf("\t4) ./ntttcp -s 192.168.1.1 -P 64 -n 16 -l 10 -f25001 -6 -V\n");
	printf("\t3) ./ntttcp -s 192.168.1.1 --fq-rate-limit 10G\n");
	printf("\t4) ./ntttcp -s 192.168.1.1 -B 10G\n");
	printf("\t4) ./ntttcp -s 192.168.1.1 --show-tcp-retrans --show-nic-packets eth0 --show-dev-interrupts mlx4 -V\n");
}

void print_version()
{
	printf("%s %s\n", TOOL_NAME, TOOL_VERSION);
	printf("---------------------------------------------------------\n");
}

int process_mappings(struct ntttcp_test *test)
{
	int state = S_THREADS, threads = 0;
	char *token = NULL;
	int cpu = -1, total_cpus = 0;

	state = S_THREADS;
	char *element = strdup(test->mapping);

	while ((token = strsep(&element, ",")) != NULL) {
		if (S_THREADS == state) {
			threads = atoi(token);

			if (1 > threads) {
				return ERROR_ARGS;
			}
			test->server_ports = threads;
			test->threads_per_server_port = 1;
			++state;
		} else if (S_PROCESSOR == state) {
			if (0 == strcmp(token, "*")) {
				/* do nothing */
			} else {
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
		} else if (S_HOST == state) {
			test->bind_address = token;
			++state;
		} else {
			PRINT_ERR("process_mappings: unexpected parameters in mapping");
			return ERROR_ARGS;
		}
	}
	return NO_ERROR;
}

/* Check flag or role compatibility; set default value for some params */
int verify_args(struct ntttcp_test *test)
{
	if (test->server_role && test->client_role) {
		PRINT_ERR("both sender and receiver roles provided");
		return ERROR_ARGS;
	}

	if (!strcmp(test->mapping, "")) {
		PRINT_ERR("no mapping provided");
		return ERROR_ARGS;
	}

	if (test->domain == AF_INET6 && strcmp(test->bind_address, "0.0.0.0") == 0)
		test->bind_address = "::";

	if (test->domain == AF_INET6 && !strstr(test->bind_address, ":")) {
		PRINT_ERR("invalid ipv6 address provided");
		return ERROR_ARGS;
	}

	if (test->domain == AF_INET && !strstr(test->bind_address, ".")) {
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

	if (test->client_role) {
		if (test->use_epoll)
			PRINT_DBG("ignore '-e' on sender role");
		if (!test->exit_after_done)
			PRINT_DBG("ignore '-H' on sender role");
	}

	if (test->server_role) {
		if (test->client_base_port > 0)
			PRINT_DBG("ignore '-f' on receiver role");
		if (test->bandwidth_limit != 0)
			PRINT_DBG("ignore '-B' on receiver role");
	}

	if (test->client_role) {
		if (test->client_base_port > 0 && test->client_base_port <= 1024) {
			test->client_base_port = DEFAULT_BASE_SRC_PORT;
			PRINT_DBG("source port is too small. use the default value");
		}

		if ((int)(MAX_LOCAL_IP_PORT - test->client_base_port) <
			(int)(test->server_ports * test->threads_per_server_port * test->conns_per_thread)) {
			PRINT_ERR("source port is too high to provide a sufficient range of ports for your test");
		}

		if (test->bandwidth_limit != 0)
			PRINT_INFO("Warning! '-B' is specified and the network bandwidth may be limited");

		if (test->fq_rate_limit != 0)
			PRINT_INFO("Warning! '--fq-rate-limit' is specified and the network bandwidth may be limited");

		if (test->bandwidth_limit != 0 && test->fq_rate_limit != 0) {
			PRINT_INFO("ignore '--fq-rate-limit' as '-B' is specified");
			test->fq_rate_limit = 0;
		}
	}

	if (test->protocol == UDP && test->send_buf_size > MAX_UDP_SEND_SIZE) {
		PRINT_INFO("the UDP send size is too big. use the max value");
		test->send_buf_size = MAX_UDP_SEND_SIZE;
	}

	if (test->show_interface_packets[0] == '-') {
		PRINT_INFO("invalid network interface provided. ignore it");
		test->show_interface_packets = "";
	}

	if (test->show_dev_interrupts[0] == '-') {
		PRINT_INFO("invalid interrupt device provided. ignore it");
		test->show_dev_interrupts = "";
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

	if (test->cooldown < 0) {
		test->cooldown = DEFAULT_COOLDOWN_SEC;
		PRINT_INFO("invalid test cool-down seconds provided. use the default value");
	}

	return NO_ERROR;
}

int parse_arguments(struct ntttcp_test *test, int argc, char **argv)
{
	/* long options, for uncommon usage */
	static struct option longopts[] = {
		{"show-tcp-retrans", no_argument, NULL, LO_SHOW_TCP_RETRANS},
		{"show-nic-packets", required_argument, NULL, LO_SHOW_NIC_PACKETS},
		{"show-dev-interrupts", required_argument, NULL, LO_SHOW_DEV_INTERRUPTS},
		{"fq-rate-limit", required_argument, NULL, LO_FQ_RATE_LIMIT},
		{0, 0, 0, 0}
	};
	int opt;

	while ((opt = getopt_long(argc, argv, "r::s::DMLeHm:P:n:l:6up:f::b:B:W:t:C:NO::x::j::QVh", longopts, NULL)) != -1) {
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
				if (optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0])
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
			test->recv_buf_size = unit_atod(optarg, BINARY_BASED_UNIT_K);
			test->send_buf_size = unit_atod(optarg, BINARY_BASED_UNIT_K);
			break;

		case 'B':
			test->bandwidth_limit = unit_atod(optarg, DECIMAL_BASED_UNIT_K);
			break;

		case LO_FQ_RATE_LIMIT:
			test->fq_rate_limit = unit_atod(optarg, DECIMAL_BASED_UNIT_K);
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

		case LO_SHOW_TCP_RETRANS:
			test->show_tcp_retransmit = true;
			break;

		case LO_SHOW_NIC_PACKETS:
			test->show_interface_packets = optarg;
			break;

		case LO_SHOW_DEV_INTERRUPTS:
			test->show_dev_interrupts = optarg;
			break;

		case 'O':
			test->save_console_log = true;
			if (optarg) {
				test->console_log_filename = optarg;
			} else {
				if (optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0])
					test->console_log_filename = argv[optind++];
			}
			break;

		case 'x':
			test->save_xml_log = true;
			if (optarg) {
				test->xml_log_filename = optarg;
			} else {
				if (optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0])
					test->xml_log_filename = argv[optind++];
			}
			break;

		case 'j':
			test->save_json_log = true;
			if (optarg) {
				test->json_log_filename = optarg;
			} else {
				if (optind < argc && NULL != argv[optind] && '\0' != argv[optind][0] && '-' != argv[optind][0])
					test->json_log_filename = argv[optind++];
			}
			break;

		case 'Q':
			test->quiet = true;
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
