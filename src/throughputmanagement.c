// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "throughputmanagement.h"

struct report_segment report_real_time_throughput(struct ntttcp_test_endpoint *tep,
						  struct report_segment last_checkpoint,
						  uint total_test_threads)
{
	struct report_segment this_checkpoint;

	uint64_t this_total_bytes = 0;
	uint64_t last_total_bytes = last_checkpoint.bytes;
	uint64_t total_bytes = 0;

	struct timeval this_check_time;
	struct timeval last_check_time = last_checkpoint.time;
	double test_time = 0;
	uint n = 0;

	for (n = 0; n < total_test_threads; n++) {
		if (tep->test->client_role == true )
			this_total_bytes += tep->client_streams[n]->total_bytes_transferred;
		else
			this_total_bytes += tep->server_streams[n]->total_bytes_transferred;
	}
	gettimeofday(&this_check_time, NULL);
	test_time = get_time_diff(&this_check_time, &last_check_time);
	total_bytes = this_total_bytes - last_total_bytes;

	printf("%c[2K", 27); /* cleanup current line */
	printf("%s: %s\r", "Real-time throughput", format_throughput(total_bytes, test_time));
	fflush(stdout);

	this_checkpoint.interval_sec = test_time;
	this_checkpoint.time  = this_check_time;
	this_checkpoint.bytes = this_total_bytes;

	return this_checkpoint;
}

void run_ntttcp_throughput_management(struct ntttcp_test_endpoint *tep)
{
	uint n = 0;
	uint i = 0;
	double elapsed_sec = 0.0;

	struct timeval now;
	double actual_test_time = 0;

	struct report_segment last_checkpoint;

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
	if (tep->test->warmup > 0 ) {
		gettimeofday(&last_checkpoint.time, NULL);
		last_checkpoint.bytes = 0;
		elapsed_sec = 0;
		while (elapsed_sec < (double)tep->test->warmup) {
			/* wait 0.5 second, or 1000 test status poll cycles.
			 * we don't roll up the warmup bytes into final throughput report,
			 * so we don't care the time accuracy */
			usleep(THROUGHPUT_INTERVAL_U_SEC);	/* 500000 micro-seconds */	
			last_checkpoint = report_real_time_throughput(tep,
					                               last_checkpoint,
								       total_test_threads);

			elapsed_sec += last_checkpoint.interval_sec;

			/* if test was interrupted by CTRL + C */
			if (!is_light_turned_on(tep->test->duration == 0)) {
				PRINT_INFO("Test was interrupted.");
				goto END;
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
	elapsed_sec = 0;		/* reset the counter */
	last_checkpoint.bytes = 0;	/* the counters in streams have been reset to 0 above */
	gettimeofday(&now, NULL);	/* reset the timestamp to now */
	last_checkpoint.time = now;
	tep->start_time = now;

	/* calculate the initial resource usage */
	get_cpu_usage( tep->results->init_cpu_usage );
	get_cpu_usage_from_proc_stat( tep->results->init_cpu_ps );
	get_tcp_retrans( tep->results->init_tcp_retrans );
	tep->results->init_tx_packets = get_single_value_from_os_file(tep->test->show_interface_packets, "tx");
	tep->results->init_rx_packets = get_single_value_from_os_file(tep->test->show_interface_packets, "rx");
	tep->results->init_interrupts = get_interrupts_from_proc_by_dev(tep->test->show_dev_interrupts);

	while(is_light_turned_on(tep->test->duration == 0)) {
		/* Wait 500 micro-seconds. We don't want to pull the status too often.
		 * But, we also don't want to wait too long time;
		 * otherwise, in the case of CTRL+C, the test streams have been stopped by CTRL+C (then light is turned off), but we are still waiting here
		 * which will make the "actual_test_time" longer than actual stream run time,
		 * and eventually make the final throughput reported inaccurate (will be lower than actual throughput)
		 */
		usleep(TEST_STATUS_POLL_INTERVAL_U_SEC);

		/* if we have already waited 1000 poll times (0.5 second), then let's report the throughput */
		i++;
		if (i == THROUGHPUT_INTERVAL_POLLS) {
			last_checkpoint = report_real_time_throughput(tep,
								      last_checkpoint,
								      total_test_threads);
			i = 0; //reset the counter
			elapsed_sec += last_checkpoint.interval_sec;
		}

		/* if test duration time is reached, then exit this stage */
		if (elapsed_sec > tep->test->duration)
			break;
	}

	/* calculate the end resource usage */
	get_cpu_usage( tep->results->final_cpu_usage );
	get_cpu_usage_from_proc_stat( tep->results->final_cpu_ps );
	get_tcp_retrans( tep->results->final_tcp_retrans );
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

	/* 3) wait, if cool-down specified. there are some possibilities:
	 *    a) "-C" is specified by user, then Cooldown here.
	 *    b) "-C" is not specified (Cooldown time is 0), but the total test cycle time negotiated with remote peer,
	 *    is longer than the local one, then the excess time will be treated as Cooldown.
	 *    Example:
	 *    This endpoint   is running with: warmup:  5, duration: 60, cooldown:  0
	 *    Remote endpoint is running with: warmup: 10, duration: 90, cooldown: 10
	 *    Then, this endpoint will have 110 secs of test cycle time (total time), which is negotiated with remote endpoint (use the max one);
	 *    Then, this endpoint will have 45 seconds of cooldown time (= 110 - 5 - 60) */
	if (tep->negotiated_test_cycle_time - tep->test->warmup - tep->test->duration > 0)
		PRINT_INFO("Test cooldown is in progress...")

	wait_light_off();
	PRINT_INFO("Test cycle finished.");

END:
	if (tep->test->client_role == true) {
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
