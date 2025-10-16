// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "throughputmanagement.h"

void check_bandwidth_limit(struct ntttcp_test_endpoint *tep)
{
	/*
	 * Return immediately in the case of:
	 * a) this is receiver (we don't limit receiver);
	 * b) this is sender, but user did not specify the bandwidth limit.
	 */
	if ((tep->endpoint_role == ROLE_RECEIVER) || (tep->test->bandwidth_limit == 0))
		return;

	struct timeval this_check_time;
	double test_time_elapsed;
	ulong current_throughput;
	uint n = 0;

	gettimeofday(&this_check_time, NULL);
	test_time_elapsed = get_time_diff(&this_check_time, &tep->start_time);

	for (n = 0; n < tep->total_threads; n++) {
		/* No need to check the current sc is the sync_thread or not,
		 * because sender does not have a delegated thread for sync
		 */

		current_throughput = (tep->client_streams[n]->total_bytes_transferred) / test_time_elapsed;
		if (current_throughput > tep->client_streams[n]->sc_bandwidth_limit_bytes)
			tep->client_streams[n]->hold_on = true;
		else
			tep->client_streams[n]->hold_on = false;
	}
}

void run_ntttcp_throughput_management(struct ntttcp_test_endpoint *tep)
{
	uint n = 0;
	struct timeval now;
	double actual_test_time = 0;
	uint64_t total_bytes_warmup = 0;
	uint64_t total_bytes_duration = 0;
	uint64_t nbytes;
	uint total_test_threads = tep->total_threads;

	/* light is already turned on before entering this function */

	/* Now the ntttcp test traffic is running now */
	tep->state = TEST_RUNNING;

	/* run the timer. it will trigger turn_off_light() after timer timeout */
	run_test_timer(tep->negotiated_test_cycle_time);

	/* 1) run test warm-up, if it is specified */
	if (tep->test->warmup > 0) {

		/*Sleep for warm-up duration*/
		struct timespec req, rem;
		req.tv_sec = tep->test->warmup;
		
		while(nanosleep(&req, &rem) == -1){
			if (errno == EINTR) {
				req = rem;
			} else {
				fprintf(stderr, "Unexpected error (errno %d): %s\n", errno, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		
		PRINT_INFO("Test warmup completed.");
		/*
		 * 1) reset each stream's total_bytes_transferred counter to 0 (discard warmup bytes)
		 * 2) do a simple stats for how many bytes transferred in this warmup stage
		 */
		for (n = 0; n < total_test_threads; n++) {
			if (tep->test->client_role == true) {
				total_bytes_warmup += (uint64_t)__sync_lock_test_and_set(&(tep->client_streams[n]->total_bytes_transferred), 0);
			} else {
				/* exclude the sync thread (only receiver has a thread for sync) */
				if (tep->server_streams[n]->is_sync_thread == true)
					continue;

				total_bytes_warmup += (uint64_t)__sync_lock_test_and_set(&(tep->server_streams[n]->total_bytes_transferred), 0);
			}
		}
	}

	/* 2) now, let's run the real test duration */
	gettimeofday(&now, NULL); /* reset the timestamp to now */
	tep->start_time = now;

	/* calculate the initial resource usage */
	get_cpu_usage(tep->results->init_cpu_usage);
	get_cpu_usage_from_proc_stat(tep->results->init_cpu_ps);
	get_tcp_retrans(tep->results->init_tcp_retrans);
	tep->results->init_tx_packets = get_single_value_from_os_file(tep->test->show_interface_packets, "tx");
	tep->results->init_rx_packets = get_single_value_from_os_file(tep->test->show_interface_packets, "rx");
	tep->results->init_interrupts = get_interrupts_from_proc_by_dev(tep->test->show_dev_interrupts);

	/*Sleep for the test duration*/
	struct timespec req, rem;
	req.tv_sec = tep->test->duration;
	
	while (nanosleep(&req, &rem) == -1){
		if (errno == EINTR) {
			req = rem;
		} else {
			fprintf(stderr, "Unexpected error (errno %d): %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	/* calculate the end resource usage */
	get_cpu_usage(tep->results->final_cpu_usage);
	get_cpu_usage_from_proc_stat(tep->results->final_cpu_ps);
	get_tcp_retrans(tep->results->final_tcp_retrans);
	tep->results->final_tx_packets = get_single_value_from_os_file(tep->test->show_interface_packets, "tx");
	tep->results->final_rx_packets = get_single_value_from_os_file(tep->test->show_interface_packets, "rx");
	tep->results->final_interrupts = get_interrupts_from_proc_by_dev(tep->test->show_dev_interrupts);

	gettimeofday(&now, NULL);
	tep->end_time = now;

	/* calculate the actual test run time */
	actual_test_time = get_time_diff(&tep->end_time, &tep->start_time);

	for (n = 0; n < total_test_threads; n++) {
		if (tep->test->client_role == true) {
			nbytes = (uint64_t)__sync_lock_test_and_set(&(tep->client_streams[n]->total_bytes_transferred), 0);
		} else {
			/* exclude the sync thread (only receiver has a thread for sync) */
			if (tep->server_streams[n]->is_sync_thread == true)
				continue;

			nbytes = (uint64_t)__sync_lock_test_and_set(&(tep->server_streams[n]->total_bytes_transferred), 0);
		}

		tep->results->threads[n]->total_bytes = nbytes;
		tep->results->threads[n]->actual_test_time = actual_test_time;
		total_bytes_duration += nbytes;
	}

	PRINT_INFO("Test run completed.");

	tep->results->total_bytes = total_bytes_duration;
	tep->results->actual_test_time = actual_test_time;

	/*
	 * 3) wait, if cool-down specified. there are some possibilities:
	 *    a) "-C" is specified by user, then Cooldown here.
	 *    b) "-C" is not specified (Cooldown time is 0), but the total test cycle time negotiated with remote peer,
	 *    is longer than the local one, then the excess time will be treated as Cooldown.
	 *    Example:
	 *    This endpoint   is running with: warmup:  5, duration: 60, cooldown:  0
	 *    Remote endpoint is running with: warmup: 10, duration: 90, cooldown: 10
	 *    Then, this endpoint will have 110 secs of test cycle time (total time), which is negotiated with remote endpoint (use the max one);
	 *    Then, this endpoint will have 45 seconds of cooldown time (= 110 - 5 - 60)
	 */
	if (tep->negotiated_test_cycle_time - tep->test->warmup - tep->test->duration > 0)
		PRINT_INFO("Test cooldown is in progress...");

	wait_light_off();
	PRINT_INFO("Test cycle finished.");

		if (tep->test->client_role == true && tep->test->no_synch == false) {
		/*
		 * if actual_test_time < tep->negotiated_test_cycle_time;
		 * then this indicates that in the sender side, test is being interrupted.
		 * hence, tell receiver about this.
		 */
		if (actual_test_time < tep->negotiated_test_cycle_time) {
			tell_receiver_test_exit(tep->synch_socket);
		}
		close(tep->synch_socket);
	}

	tep->state = TEST_FINISHED;
	return;
}