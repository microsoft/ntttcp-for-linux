// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "parameter.h"
#include "tcpstream.h"

#include <assert.h>

static struct ntttcp_test_endpoint *receive_endpoint;

/* Inject a reset after the receiver accepts a readable connection. */
ssize_t __wrap_recv(int fd, void *buffer, size_t length, int flags)
{
	(void)fd;
	(void)buffer;
	(void)length;
	(void)flags;
	receive_endpoint->state = TEST_FINISHED;
	errno = ECONNRESET;
	return -1;
}

static void test_receive_error(bool use_epoll)
{
	struct ntttcp_test_endpoint endpoint = {
		.receiver_exit_after_done = true,
		.state = TEST_RUNNING
	};
	struct ntttcp_stream_server stream = {
		.endpoint = &endpoint,
		.domain = AF_INET,
		.protocol = TCP,
		.bind_address = "127.0.0.1",
		.recv_buf_size = 1024
	};
	struct sockaddr_in address;
	socklen_t address_size = sizeof(address);
	int client, result;

	receive_endpoint = &endpoint;
	assert(ntttcp_server_listen(&stream) >= 0);
	assert(getsockname(stream.listener, (struct sockaddr *)&address, &address_size) == 0);
	client = socket(AF_INET, SOCK_STREAM, 0);
	assert(client >= 0);
	assert(connect(client, (struct sockaddr *)&address, address_size) == 0);
	assert(send(client, "x", 1, 0) == 1);

	result = use_epoll ? ntttcp_server_epoll(&stream) : ntttcp_server_select(&stream);
	close(client);
	printf("RESULT %d %" PRIu64 "\n", result, stream.total_bytes_transferred);
}

int main(int argc, char **argv)
{
	if (argc == 2 && strcmp(argv[1], "receive-select") == 0) {
		test_receive_error(false);
	} else if (argc == 2 && strcmp(argv[1], "receive-epoll") == 0) {
		test_receive_error(true);
	} else if (argc == 5 && strcmp(argv[1], "counter") == 0) {
		printf("RESULT %" PRIu64 "\n", read_counter_from_proc(argv[2], argv[3], argv[4]));
	} else if (argc >= 2 && strcmp(argv[1], "args") == 0) {
		struct ntttcp_test *test = new_ntttcp_test();
		int result;

		assert(test != NULL);
		assert(default_ntttcp_test(test) == NO_ERROR);
		result = parse_arguments(test, argc - 1, argv + 1);
		if (result == NO_ERROR)
			result = verify_args(test);
		free(test->bind_address);
		free(test);
		printf("RESULT %d\n", result);
	} else {
		return 1;
	}
	return 0;
}
