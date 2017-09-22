// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "ntttcp.h"

struct ntttcp_test *new_ntttcp_test()
{
	struct ntttcp_test *test;
	test = (struct ntttcp_test *) malloc(sizeof(struct ntttcp_test));
	if (!test)
		return NULL;

	memset(test, 0, sizeof(struct ntttcp_test));
	return test;
}

void default_ntttcp_test(struct ntttcp_test *test)
{
	test->server_role      = false;
	test->client_role      = false;
	test->daemon           = false;
	test->multi_clients_mode  = false;
	test->last_client      = false;
	test->use_epoll        = false;
	test->mapping          = "16,*,*";
	test->bind_address     = "0.0.0.0";
	test->parallel         = DEFAULT_NUM_THREADS;
	test->cpu_affinity     = -1; //no hard cpu affinity
	test->conn_per_thread  = DEFAULT_CONN_PER_THREAD;
	test->domain           = AF_INET; //IPV4
	test->protocol         = TCP;
	test->server_base_port = DEFAULT_BASE_DST_PORT;
	test->client_base_port = 0;                        //random/ephemeral port
	test->recv_buf_size    = DEFAULT_RECV_BUFFER_SIZE; //64K
	test->send_buf_size    = DEFAULT_SEND_BUFFER_SIZE; //128K
	test->duration         = DEFAULT_TEST_DURATION;
	test->no_synch         = false;
	test->show_tcp_retransmit = false;
	test->verbose          = false;
}

struct ntttcp_test_endpoint *new_ntttcp_test_endpoint(struct ntttcp_test *test, int endpoint_role)
{
	int i = 0;
	struct timeval now;
	int total_threads = 0;

	struct ntttcp_test_endpoint *e;
	e = (struct ntttcp_test_endpoint *) malloc(sizeof(struct ntttcp_test_endpoint ));
	if(!e)
		return NULL;

	gettimeofday(&now, NULL);

	memset(e, 0, sizeof(struct ntttcp_test_endpoint));
	e->endpoint_role = endpoint_role;
	e->test = test;
	e->state = TEST_NOT_STARTED;
	e->confirmed_duration = test->duration;
	e->start_time = now;
	e->end_time = now;
	e->synch_socket = 0;
	e->num_remote_endpoints = 0;
	memset(e->remote_endpoints, -1, MAX_REMOTE_ENDPOINTS);

	if (endpoint_role == ROLE_SENDER) {
		if (test->no_synch == true)
			total_threads = test->parallel * test->conn_per_thread;
		else
			total_threads = test->parallel * test->conn_per_thread + 1;

		e->total_threads = total_threads;
		e->client_streams = (struct ntttcp_stream_client **) malloc( sizeof( struct  ntttcp_stream_client ) * total_threads );
		if(!e->client_streams) {
			free (e);
			return NULL;
		}
		memset(e->client_streams, 0, sizeof( struct  ntttcp_stream_client ) * total_threads );
		for(i = 0; i < total_threads ; i++ ) {
			e->client_streams[i] = new_ntttcp_client_stream(test);
		}

		e->threads = malloc( total_threads * sizeof(pthread_t) );
		if(!e->threads) {
			for(i = 0; i < total_threads ; i++ ){
				free( e->client_streams[i] );
			}
			free (e->client_streams);
			free (e);
			return NULL;
		}
	}
	else {
		if (test->no_synch == true)
			total_threads = test->parallel;
		else
			total_threads = test->parallel + 1;

		e->total_threads = total_threads;
		e->server_streams = (struct  ntttcp_stream_server **) malloc( sizeof( struct  ntttcp_stream_server ) * total_threads );
		if(!e->server_streams) {
			free (e);
			return NULL;
		}
		memset(e->server_streams, 0, sizeof( struct  ntttcp_stream_server) * total_threads );
		for(i = 0; i < total_threads; i++ ){
			e->server_streams[i] = new_ntttcp_server_stream(test);
		}

		e->threads = malloc( total_threads * sizeof(pthread_t) );
		if(!e->threads) {
			for(i = 0; i < total_threads; i++ ){
				free( e->server_streams[i] );
			}
			free (e->server_streams);
			free (e);
			return NULL;
		}
	}
	return e;
}

void set_ntttcp_test_endpoint_test_continuous(struct ntttcp_test_endpoint* e)
{
	int i;

	if (e->endpoint_role == ROLE_SENDER)
		for (i = 0; i < e->total_threads; i++)
			e->client_streams[i]->continuous_mode = true;
	else
		for (i = 0; i < e->total_threads; i++)
			e->server_streams[i]->continuous_mode = true;
}

void free_ntttcp_test_endpoint_and_test(struct ntttcp_test_endpoint* e)
{
	int i = 0;
	int total_threads = 0;
	int endpoint_role = e->endpoint_role;

	if (endpoint_role == ROLE_SENDER) {
		if (e->test->no_synch == true)
			total_threads = e->test->parallel * e->test->conn_per_thread;
		else
			total_threads = e->test->parallel * e->test->conn_per_thread + 1;

		for(i = 0; i < total_threads ; i++ ){
			free( e->client_streams[i] );
		}
		free( e->client_streams );
	}
	else {
		if (e->test->no_synch == true)
			total_threads = e->test->parallel;
		else
			total_threads = e->test->parallel + 1;

		for(i = 0; i < total_threads ; i++ ){
			free( e->server_streams[i] );
		}
		free( e->server_streams );
	}
	free( e->threads );
	free( e->test );
	free( e );
}

struct ntttcp_stream_client *new_ntttcp_client_stream(struct ntttcp_test *test)
{
	struct ntttcp_stream_client *s;
	s = (struct ntttcp_stream_client *) malloc(sizeof(struct ntttcp_stream_client));
	if (!s)
		return NULL;

	memset(s, 0, sizeof(struct ntttcp_stream_client));
	s->domain = test->domain;
	s->protocol = test->protocol;
	s->bind_address = test->bind_address;
	//s->server_port, should be specified by caller
	//s->client_port, should be specified by caller
	s->send_buf_size = test->send_buf_size;
	s->verbose = test->verbose;
	s->is_sync_thread = 0;
	s->no_synch = test->no_synch;
	s->continuous_mode = (test->duration == 0);
	s->total_bytes_transferred = 0;
	return s;
}

struct ntttcp_stream_server *new_ntttcp_server_stream(struct ntttcp_test *test)
{
	struct ntttcp_stream_server *s;
	s = (struct ntttcp_stream_server *) malloc(sizeof(struct ntttcp_stream_server));
	if (!s)
	 	return NULL;

	memset(s, 0, sizeof(struct ntttcp_stream_server));
	s->domain = test->domain;
	s->protocol = test->protocol;
	s->bind_address = test->bind_address;
	//s->server_port, should be specified by caller
	s->recv_buf_size = test->recv_buf_size;
	s->verbose = test->verbose;
	s->is_sync_thread = 0;
	s->no_synch = test->no_synch;
	s->continuous_mode = (test->duration == 0);
	s->use_epoll = test->use_epoll;
	s->total_bytes_transferred = 0;
	//other fields will be assigned at run time
	return s;
}
