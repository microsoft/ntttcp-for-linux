// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "throughputmanagement.h"

void *run_ntttcp_throughput_management(void *ptr)
{
	struct ntttcp_test_endpoint *tep = (struct ntttcp_test_endpoint *) ptr;
	uint n;
	uint64_t last_total_bytes, this_total_bytes, total_bytes;
	uint total_test_threads = 0;
	last_total_bytes = this_total_bytes = total_bytes = 0;

	if (tep->test->client_role == true )
		total_test_threads = tep->test->server_ports * tep->test->threads_per_server_port;
	else
		total_test_threads = tep->test->server_ports;

	wait_light_on();

	while( is_light_turned_on(tep->test->duration == 0) ) {
		for (n = 0; n < total_test_threads; n++) {
			if (tep->test->client_role == true )
				this_total_bytes += tep->client_streams[n]->total_bytes_transferred;
			else
				this_total_bytes += tep->server_streams[n]->total_bytes_transferred;
		}

		total_bytes = this_total_bytes - last_total_bytes;
		last_total_bytes = this_total_bytes;
		this_total_bytes = 0;

		/* sleep 0.5 second, then calculate the throughput for this period */
		printf("%s: %s\r", "Real-time throughput", format_throughput(total_bytes, 0.5));
		fflush(stdout);
		usleep(500000);
	}

	return NULL;
}
