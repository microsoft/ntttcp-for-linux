// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

/* include "ntttcp.h" */
#include "util.h"

struct ntttcp_test *new_ntttcp_test()
{
	struct ntttcp_test *test;
	test = (struct ntttcp_test *)malloc(sizeof(struct ntttcp_test));
	if (!test)
		return NULL;

	memset(test, 0, sizeof(struct ntttcp_test));
	return test;
}

void default_ntttcp_test(struct ntttcp_test *test)
{
	test->server_role		= false;
	test->client_role		= false;
	test->daemon			= false;
	test->multi_clients_mode	= false;
	test->last_client		= false;
	test->use_epoll			= false;
	test->use_iouring      = false;
	test->exit_after_done		= true;
	test->mapping			= "16,*,*";
	test->bind_address		= "0.0.0.0";
	test->cpu_affinity		= -1; /* no hard cpu affinity */
	test->server_ports		= DEFAULT_NUM_SERVER_PORTS;	 //default:16 */
	test->threads_per_server_port	= DEFAULT_THREADS_PER_SERVER_PORT; /* default: 4, sender only */
	test->conns_per_thread		= DEFAULT_CLIENT_CONNS_PER_THREAD; /* default: 1, sender only */
	test->domain			= AF_INET; /* IPV4 */
	test->protocol			= TCP;
	test->server_base_port		= DEFAULT_BASE_DST_PORT;
	test->client_base_port		= 0; /* random/ephemeral port */
	test->recv_buf_size		= DEFAULT_RECV_BUFFER_SIZE; /* 64K */
	test->send_buf_size		= DEFAULT_SEND_BUFFER_SIZE; /* 128K */
	test->bandwidth_limit		= 0; /* no bandwidth limit */
	test->fq_rate_limit		= 0; /* o fq rate limit */
	test->warmup			= DEFAULT_WARMUP_SEC; /* 0 sec */
	test->duration			= DEFAULT_TEST_DURATION; /* 60 sec */
	test->cooldown			= DEFAULT_COOLDOWN_SEC; /* 0 sec */
	test->no_synch			= false;
	test->show_tcp_retransmit	= false;
	test->show_interface_packets	= "";
	test->show_dev_interrupts	= "";
	test->save_console_log		= false;
	test->console_log_filename	= DEFAULT_CONSOLE_LOG_FILE_NAME; /* "ntttcp-for-linux-log.log" */
	test->save_xml_log		= false;
	test->xml_log_filename		= DEFAULT_XML_LOG_FILE_NAME; /* "ntttcp-for-linux-log.xml" */
	test->save_json_log		= false;
	test->json_log_filename		= DEFAULT_JSON_LOG_FILE_NAME; /* "ntttcp-for-linux-log.json" */
	test->quiet			= false;
	test->verbose			= false;
}

bool is_running_tty(void)
{
	const char *term  = getenv("TERM");
	bool is_dumb_term = term && !strcmp(term, "dumb");
	return !is_dumb_term && isatty(fileno(stdout));
}

struct ntttcp_test_endpoint *new_ntttcp_test_endpoint(struct ntttcp_test *test, int endpoint_role)
{
	unsigned int i = 0;
	struct timeval now;
	unsigned int total_threads = 0;

	struct ntttcp_test_endpoint *e;
	e = (struct ntttcp_test_endpoint *)malloc(sizeof(struct ntttcp_test_endpoint));
	if (!e)
		return NULL;

	gettimeofday(&now, NULL);

	memset(e, 0, sizeof(struct ntttcp_test_endpoint));
	e->endpoint_role		= endpoint_role;
	e->test				= test;
	e->state			= TEST_NOT_STARTED;
	e->receiver_exit_after_done	= test->exit_after_done;
	e->negotiated_test_cycle_time	= test->warmup + test->duration + test->cooldown;
	e->start_time			= now;
	e->end_time			= now;
	e->running_tty			= is_running_tty();
	e->synch_socket			= 0;
	e->num_remote_endpoints		= 0;
	for (i = 0; i < MAX_REMOTE_ENDPOINTS; i++)
		e->remote_endpoints[i] = -1;

	if (endpoint_role == ROLE_SENDER) {
		/*
		 * for sender, even used synch mechanism, the main thread will do the synch.
		 * no specially created thread for synch.
		 */
		total_threads = test->server_ports * test->threads_per_server_port;

		e->total_threads = total_threads;
		e->client_streams =
			(struct ntttcp_stream_client **)malloc(sizeof(struct ntttcp_stream_client *) * total_threads);
		if (!e->client_streams) {
			free(e);
			return NULL;
		}
		memset(e->client_streams, 0, sizeof(struct ntttcp_stream_client *) * total_threads);
		for (i = 0; i < total_threads; i++) {
			e->client_streams[i] = new_ntttcp_client_stream(e);
		}

		e->threads = malloc(total_threads * sizeof(pthread_t));
		if (!e->threads) {
			for (i = 0; i < total_threads; i++) {
				free(e->client_streams[i]);
			}
			free(e->client_streams);
			free(e);
			return NULL;
		}
	} else {
		if (test->no_synch == true)
			total_threads = test->server_ports;
		else
			total_threads = test->server_ports + 1; /* the last one is synch thread */

		e->total_threads = total_threads;
		e->server_streams = (struct ntttcp_stream_server **)malloc(sizeof(struct ntttcp_stream_server *) * total_threads);
		if (!e->server_streams) {
			free(e);
			return NULL;
		}
		memset(e->server_streams, 0, sizeof(struct ntttcp_stream_server *) * total_threads);
		for (i = 0; i < total_threads; i++) {
			e->server_streams[i] = new_ntttcp_server_stream(e);
		}

		e->threads = malloc(total_threads * sizeof(pthread_t));
		if (!e->threads) {
			for (i = 0; i < total_threads; i++) {
				free(e->server_streams[i]);
			}
			free(e->server_streams);
			free(e);
			return NULL;
		}
	}

	/* for test results */
	e->results = (struct ntttcp_test_endpoint_results *)malloc(sizeof(struct ntttcp_test_endpoint_results));
	memset(e->results, 0, sizeof(struct ntttcp_test_endpoint_results));
	e->results->endpoint = e;
	e->results->average_rtt = (unsigned int)-1;

	e->results->threads = (struct ntttcp_test_endpoint_thread_result **)malloc(sizeof(struct ntttcp_test_endpoint_thread_result *) * total_threads);
	memset(e->results->threads, 0, sizeof(struct ntttcp_test_endpoint_thread_result *) * total_threads);

	for (i = 0; i < total_threads; i++) {
		e->results->threads[i] = (struct ntttcp_test_endpoint_thread_result *)malloc(sizeof(struct ntttcp_test_endpoint_thread_result));

		/*
		 * For receiver role, if synch mechanism is being used (the last thread in the list is synch thread),
		 * then mark that one;
		 * Note: for sender role, if synch mechanism is being used, we will use the main thread to sync with receiver,
		 * so there is no specially created thread for synch;
		 * all threads are testing threads which will transfer test data
		 */
		if (i == (total_threads - 1) && e->endpoint_role == ROLE_RECEIVER && e->test->no_synch == false)
			e->results->threads[i]->is_sync_thread = true;
		else
			e->results->threads[i]->is_sync_thread = false;
	}

	/* for calculate the resource utilization */
	e->results->init_cpu_usage	  = (struct cpu_usage *)malloc(sizeof(struct cpu_usage));
	e->results->final_cpu_usage	  = (struct cpu_usage *)malloc(sizeof(struct cpu_usage));
	e->results->init_cpu_ps		  = (struct cpu_usage_from_proc_stat *)malloc(sizeof(struct cpu_usage_from_proc_stat));
	e->results->final_cpu_ps	  = (struct cpu_usage_from_proc_stat *)malloc(sizeof(struct cpu_usage_from_proc_stat));

	/* for calculate the TCP re-transmit */
	e->results->init_tcp_retrans  = (struct tcp_retrans *)malloc(sizeof(struct tcp_retrans));
	e->results->final_tcp_retrans = (struct tcp_retrans *)malloc(sizeof(struct tcp_retrans));

	return e;
}

void set_ntttcp_test_endpoint_test_continuous(struct ntttcp_test_endpoint *e)
{
	unsigned int i;

	if (e->endpoint_role == ROLE_SENDER)
		for (i = 0; i < e->total_threads; i++)
			e->client_streams[i]->continuous_mode = true;
	else
		for (i = 0; i < e->total_threads; i++)
			e->server_streams[i]->continuous_mode = true;
}

void free_ntttcp_test_endpoint_and_test(struct ntttcp_test_endpoint *e)
{
	uint i = 0;
	uint total_threads = 0;
	int endpoint_role  = e->endpoint_role;

	if (endpoint_role == ROLE_SENDER) {
		total_threads = e->test->server_ports * e->test->threads_per_server_port;

		for (i = 0; i < total_threads; i++)
			free(e->client_streams[i]);

		free(e->client_streams);
	} else {
		if (e->test->no_synch == true)
			total_threads = e->test->server_ports;
		else
			total_threads = e->test->server_ports + 1;

		for (i = 0; i < total_threads; i++)
			free(e->server_streams[i]);

		free(e->server_streams);
	}

	for (i = 0; i < total_threads; i++)
		free(e->results->threads[i]);
	free(e->results->init_cpu_usage);
	free(e->results->init_cpu_ps);
	free(e->results->init_tcp_retrans);
	free(e->results->final_cpu_usage);
	free(e->results->final_cpu_ps);
	free(e->results->final_tcp_retrans);
	free(e->results);
	free(e->threads);
	free(e->test);
	free(e);
}

struct ntttcp_stream_client *new_ntttcp_client_stream(struct ntttcp_test_endpoint *ept)
{
	struct ntttcp_stream_client *s;
	struct ntttcp_test *test = ept->test;

	s = (struct ntttcp_stream_client *)malloc(sizeof(struct ntttcp_stream_client));
	if (!s)
		return NULL;

	memset(s, 0, sizeof(struct ntttcp_stream_client));
	s->endpoint			= ept;
	s->domain			= test->domain;
	s->protocol			= test->protocol;
	s->bind_address 		= test->bind_address;
	s->num_connections		= test->conns_per_thread;
	s->send_buf_size		= test->send_buf_size;
	s->sc_bandwidth_limit_bytes	= test->bandwidth_limit / (test->server_ports * test->threads_per_server_port) / 8;
	s->socket_fq_rate_limit_bytes	= test->fq_rate_limit / (test->server_ports * test->threads_per_server_port * test->conns_per_thread) / 8;
	s->hold_on			= false;
	s->verbose			= test->verbose;
	s->is_sync_thread		= false;
	s->no_synch			= test->no_synch;
	s->continuous_mode		= (test->duration == 0);

	s->num_conns_created		= 0;
	s->total_bytes_transferred	= 0;
	s->average_rtt			= (uint)-1;
	return s;
}

struct ntttcp_stream_server *new_ntttcp_server_stream(struct ntttcp_test_endpoint *ept)
{
	struct ntttcp_stream_server *s;
	struct ntttcp_test *test = ept->test;

	s = (struct ntttcp_stream_server *)malloc(sizeof(struct ntttcp_stream_server));
	if (!s)
		return NULL;

	memset(s, 0, sizeof(struct ntttcp_stream_server));
	s->endpoint		= ept;
	s->domain		= test->domain;
	s->protocol		= test->protocol;
	s->bind_address 	= test->bind_address;
	/* s->server_port, should be specified by caller */
	s->recv_buf_size	= test->recv_buf_size;
	s->verbose		= test->verbose;
	s->is_sync_thread	= false;
	s->no_synch		= test->no_synch;
	s->continuous_mode	= (test->duration == 0);
	s->use_epoll		= test->use_epoll;
	s->use_iouring 		= test->use_iouring;
	s->total_bytes_transferred = 0;
	/* other fields will be assigned at run time */
	return s;
}
