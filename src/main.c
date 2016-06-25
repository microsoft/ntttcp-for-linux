// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "main.h"

/************************************************************/
//		ntttcp helper, to count CPU cycle
/************************************************************/

#if defined(__i386__)
static __inline__ unsigned long long get_cc_rdtsc(void)
{
	unsigned long long int c;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (c));
	return c;
}

#elif defined(__x86_64__)
static __inline__ unsigned long long get_cc_rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif


/************************************************************/
//		ntttcp high level functions
/************************************************************/
int run_ntttcp_sender(struct ntttcp_test_endpoint *tep)
{
	int err_code = NO_ERROR;
	struct ntttcp_test *test = tep->test;
	char *log = NULL;
	bool verbose_log = test->verbose;

	int threads_created = 0, stream_created = 0;
	struct ntttcp_stream_client *cs;
	int rc, t, i, reply_received;
	void *bytes;
	long nbytes = 0, total_bytes = 0;
	struct timeval now;
	double actual_test_time = 0;
	struct cpu_usage *init_cpu_usage, *final_cpu_usage;
	uint64_t init_cycle_count = 0, final_cycle_count = 0, cycle_diff = 0;

	/* for calculate the resource usage */
	init_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!init_cpu_usage) {
		PRINT_ERR("sender: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}
	final_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!final_cpu_usage) {
		free (init_cpu_usage);
		PRINT_ERR("sender: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}

	if (test->no_synch == false ) {
		/* Negotiate with receiver on:
		* 1) receiver state: is receiver busy with another test?
		* 2) submit sender's test duration time to receiver to negotiate
		* 3) request receiver to start the test
		*/
		reply_received = create_sender_sync_socket( tep );
		if (reply_received == 0) {
			PRINT_ERR("sender: failed to create sync socket");
			return ERROR_GENERAL;
		}
		tep->synch_socket = reply_received;
		reply_received = query_receiver_busy_state(tep->synch_socket);
		if (reply_received == -1) {
			PRINT_ERR("sender: failed to query receiver state");
			return ERROR_GENERAL;
		}
		if (reply_received == 1) {
			PRINT_ERR("sender: receiver is busy with another test");
			return ERROR_GENERAL;
		}
		reply_received = negotiate_test_duration(tep->synch_socket, test->duration);
		if (reply_received == -1) {
			PRINT_ERR("sender: failed to negotiate test duration with receiver");
			return ERROR_GENERAL;
		}
		if (reply_received != test->duration) {
			asprintf(&log, "test duration negotiated is: %d seconds", reply_received);
			PRINT_INFO_FREE(log);
		}
		tep->confirmed_duration = reply_received;
		reply_received = request_to_start(tep->synch_socket);
		if (reply_received == -1) {
			PRINT_ERR("sender: failed to sync with receiver to start test");
			return ERROR_GENERAL;
		}
		if (reply_received == 0) {
			PRINT_ERR("sender: receiver refuse to start test right now");
			return ERROR_GENERAL;
		}

		close(tep->synch_socket);

		/* if we go here, the pre-test sync has completed */
		PRINT_INFO("Network activity progressing...");
	}
	else {
		PRINT_INFO("Starting sender activity (no sync) ...");
	}

	/* create threads */
	for (t = 0; t < test->parallel; t++) {
		cs = tep->client_streams[t];
		if (cs == NULL) {
			PRINT_ERR("sender: error when creating new client stream");
			err_code = ERROR_MEMORY_ALLOC;
			continue;
		}
		cs->server_port = test->server_base_port + t;
		stream_created++;

		/* in client side, multiple connections will (one thread for one connection)
		 * connect to same port on server
		 */
		for (i = 0; i < test->conn_per_thread; i++ ) {
			if (test->protocol == TCP) {
				rc = pthread_create(&tep->data_threads[threads_created],
							NULL,
							run_ntttcp_sender_tcp_stream,
							(void*)cs);
			}
			else {
				rc = pthread_create(&tep->data_threads[threads_created],
							NULL,
							run_ntttcp_sender_udp_stream,
							(void*)cs);
			}

			if (rc) {
				PRINT_ERR("pthread_create() create thread failed");
				err_code = ERROR_PTHREAD_CREATE;
				continue;
			}
			else{
				threads_created++;
			}
		}
	}
	asprintf(&log, "%d threads created", threads_created);
	PRINT_DBG_FREE(log);

	turn_on_light();

	get_cpu_usage( init_cpu_usage );
	init_cycle_count = get_cc_rdtsc();

	/* run the timer. it will trigger turn_off_light() after timer timeout */
	run_test_timer(tep->confirmed_duration );
	tep->state = TEST_RUNNING;
	gettimeofday(&now, NULL);
	tep->start_time = now;

	/* wait test done */
	wait_light_off();
	tep->state = TEST_FINISHED;
	gettimeofday(&now, NULL);
	tep->end_time = now;

	/* calculate the actual test run time */
	actual_test_time = get_time_diff(&tep->end_time, &tep->start_time);

	/* calculate resource usage */
	final_cycle_count = get_cc_rdtsc();
	cycle_diff = final_cycle_count - init_cycle_count;
	get_cpu_usage( final_cpu_usage );

	/* calculate client side throughput, but exclude the last thread as it is synch thread */
	print_thread_result(-1, 0, 0);
	for (i = 0; i < threads_created; i++) {
		pthread_join(tep->data_threads[i], &bytes);
		nbytes = (long)bytes;
		total_bytes += nbytes;
		print_thread_result(i, nbytes, actual_test_time);
	}
	print_total_result(total_bytes, cycle_diff, actual_test_time, init_cpu_usage, final_cpu_usage);

	free (init_cpu_usage);
	free (final_cpu_usage);

	return err_code;
}

int run_ntttcp_receiver(struct ntttcp_test_endpoint *tep)
{
	int err_code = NO_ERROR;
	struct ntttcp_test *test = tep->test;
	char *log = NULL;
	bool verbose_log = test->verbose;

	int threads_created = 0, stream_created = 0;
	struct ntttcp_stream_server *ss;
	int rc, t;
	long nbytes = 0, total_bytes = 0;
	struct timeval now;
	double actual_test_time = 0;
	struct cpu_usage *init_cpu_usage, *final_cpu_usage;
	uint64_t init_cycle_count = 0, final_cycle_count = 0, cycle_diff = 0;

	/* for calculate the resource usage */
	init_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!init_cpu_usage) {
		PRINT_ERR("receiver: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}
	final_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!final_cpu_usage) {
		free (init_cpu_usage);
		PRINT_ERR("receiver: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}

	/* create threads */
	for (t = 0; t < test->parallel; t++) {
		ss = tep->server_streams[t];
		if (ss == NULL) {
			PRINT_ERR("receiver: error when creating new server stream");
			continue;
		}
		ss->server_port = test->server_base_port + t;
		stream_created++;

		if (test->protocol == TCP) {
			rc = pthread_create(&tep->data_threads[t],
						NULL,
						run_ntttcp_receiver_tcp_stream,
						(void*)ss);
		}
		else {
			rc = pthread_create(&tep->data_threads[t],
						NULL,
						run_ntttcp_receiver_udp_stream,
						(void*)ss);
		}

		if (rc) {
			PRINT_ERR("pthread_create() create thread failed");
			err_code = ERROR_PTHREAD_CREATE;
			continue;
		}
		threads_created++;
	}

	/* create synch thread */
	if (test->no_synch == false) {
		/* ss struct is not used in sync thread, because:
		 * we are only allowed to pass one param to the thread in pthread_create();
		 * but the information stored in ss, is not enough to be used for synch;
		 * so we pass *tep to the pthread_create().
		 * notes:
		 * 1) we will calculate the tcp port for synch stream in create_receiver_sync_socket().
		 *   the synch_port = base_port -1
		 * 2) we will assign the protocol for synch stream to TCP, always, in create_receiver_sync_socket()
		 */
		ss = tep->server_streams[test->parallel];
		ss->server_port = test->server_base_port - 1; //just for bookkeeping
		ss->protocol = TCP; //just for bookkeeping
		ss->is_sync_thread = 1;
		stream_created++;

		rc = pthread_create(&tep->data_threads[t],
				NULL,
				create_receiver_sync_socket,
				(void*)tep);
		if (rc) {
				PRINT_ERR("pthread_create() create thread failed");
			err_code = ERROR_PTHREAD_CREATE;
		}
		else {
			threads_created++;
		}
	}

	asprintf(&log, "%d threads created", threads_created);
	PRINT_DBG_FREE(log);

	while ( 1 ) {
		/* for receiver, there are two ways to trigger test start:
		 * a) if synch enabled, then sync thread will trigger loght_on after sync completed;
		 *	see create_receiver_sync_socket()
		 * b) if no synch enabled, then any tcp server accept client connections, the ligh_on will be triggered;
		 *	see ntttcp_server_epoll(), or ntttcp_server_select()
		 */
		wait_light_on();

		/* reset server side perf counters at the begining, after light-is-on
		 * this is to handle the case when: receiver in sync mode, but sender connected as no_sync mode
		 * in this case, before loght_no, the threads have some data counted already
		 */
		for (t=0; t < stream_created; t++)
			__atomic_store_n( &(tep->server_streams[t]->total_bytes_transferred), 0, __ATOMIC_SEQ_CST );

		get_cpu_usage( init_cpu_usage );
		init_cycle_count = get_cc_rdtsc();

		/* run the timer. it will trigger turn_off_light() after timer timeout */
		run_test_timer(tep->confirmed_duration);
		tep->state = TEST_RUNNING;
		gettimeofday(&now, NULL);
		tep->start_time = now;

		/* wait test done */
		wait_light_off();
		tep->state = TEST_FINISHED;
		gettimeofday(&now, NULL);
		tep->end_time = now;

		/* calculate the actual test run time */
		actual_test_time = get_time_diff(&tep->end_time, &tep->start_time);

		/* calculate resource usage */
		final_cycle_count = get_cc_rdtsc();
		cycle_diff = final_cycle_count - init_cycle_count;
		get_cpu_usage( final_cpu_usage );

		//sleep(1);  //looks like server needs more time to receive data ...

		/* calculate server side throughput */
		total_bytes = 0;
		print_thread_result(-1, 0, 0);
		for (t=0; t < stream_created; t++){
			/* exclude the sync thread */
			if (tep->server_streams[t]->is_sync_thread)
				continue;

			nbytes = (long)__atomic_load_n( &(tep->server_streams[t]->total_bytes_transferred), __ATOMIC_SEQ_CST );
			total_bytes += nbytes;
			print_thread_result(t, nbytes, actual_test_time);
		}

		print_total_result(total_bytes, cycle_diff, actual_test_time, init_cpu_usage, final_cpu_usage);
	}

	/* as receiver threads will keep listening on ports, so they will not exit */
	for (t=0; t < threads_created; t++) {
		pthread_join(tep->data_threads[t], NULL);
	}

	free (init_cpu_usage);
	free (final_cpu_usage);

	return err_code;
}

int main(int argc, char **argv)
{
	int err_code = NO_ERROR;
	cpu_set_t cpuset;
	struct ntttcp_test *test;
	struct ntttcp_test_endpoint *tep;

	/* catch SIGINT: Ctrl + C */
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		PRINT_ERR("main: error when setting the disposition of the signal SIGINT");

	print_version();
	test = new_ntttcp_test();
	if (!test) {
		PRINT_ERR("main: error when creating new test");
		exit (-1);
	}

	default_ntttcp_test(test);
	err_code = parse_arguments(test, argc, argv);
	if (err_code != NO_ERROR) {
		PRINT_ERR("main: error when parsing args");
		print_flags(test);
		free(test);
		exit (-1);
	}

	err_code = verify_args(test);
	if (err_code != NO_ERROR) {
		PRINT_ERR("main: error when verifying the args");
		print_flags(test);
		free(test);
		exit (-1);
	}

	if (test->verbose)
		print_flags(test);

	turn_off_light();

	if (test->cpu_affinity != -1) {
		CPU_ZERO(&cpuset);
		CPU_SET(test->cpu_affinity, &cpuset);
		PRINT_INFO("main: set cpu affinity");
		if ( pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t ), &cpuset) != 0 )
			PRINT_ERR("main: cannot set cpu affinity");
	}

	if (test->daemon) {
		PRINT_INFO("main: run this tool in the background");
		if ( daemon(0, 0) != 0 )
			PRINT_ERR("main: cannot run this tool in the background");
	}

	if (test->client_role == true) {
		tep = new_ntttcp_test_endpoint(test, ROLE_SENDER);
		err_code = run_ntttcp_sender(tep);
	}
	else {
		tep = new_ntttcp_test_endpoint(test, ROLE_RECEIVER);
		err_code = run_ntttcp_receiver(tep);
	}

	free_ntttcp_test_endpoint_and_test(tep);
	return err_code;
}
