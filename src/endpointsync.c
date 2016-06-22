// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "endpointsync.h"

/************************************************************/
//		ntttcp sender sync functions
/************************************************************/
int create_sender_sync_socket( struct ntttcp_test_endpoint *tep )
{
	char *log = NULL;
	int sockfd = 0; //socket id
	struct ntttcp_test *test = tep->test;
	bool verbose_log = test->verbose;

	struct sockaddr_storage local_addr; //for local address
	socklen_t local_addr_size; //local address size, for getsockname(), to get local port
	char *ip_address_str; //used to get remote peer's ip address
	int ip_address_max_size;  //used to get remote peer's ip address
	int sync_port = 0;
	char *port_str; //to get remote peer's port number for getaddrinfo()
	struct addrinfo hints, *serv_info, *p; //to get remote peer's sockaddr for connect()

	int i = 0;

	sync_port = test->server_base_port - 1;

	ip_address_max_size = (test->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return 0;
	}

	/* connect to remote receiver */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = test->domain;
	hints.ai_socktype = test->protocol;
	asprintf(&port_str, "%d", sync_port);
	if (getaddrinfo(test->bind_address, port_str, &hints, &serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return 0;
	}
	free(port_str);

	/* only get the first entry to connect */
	for (p = serv_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			PRINT_ERR("cannot create socket ednpoint");
			freeaddrinfo(serv_info);
			free(ip_address_str);
			return 0;
		}
		ip_address_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr, ip_address_str, ip_address_max_size);
		if (( i = connect(sockfd, p->ai_addr, p->ai_addrlen)) < 0) {
			if (i == -1) {
				asprintf(&log, "failed to connect to receiver: %s:%d on socket: %d. errno = %d", ip_address_str, sync_port, sockfd, errno);
				PRINT_ERR_FREE(log);
			}
			else {
				asprintf(&log, "failed to connect to receiver: %s:%d on socket: %d. error code = %d", ip_address_str, sync_port, sockfd, i);
				PRINT_ERR_FREE(log);
			}
			freeaddrinfo(serv_info);
			free(ip_address_str);
			close(sockfd);
			return 0;
		}
		else{
			break; //connected
		}
	}

	/* get local port number */
	local_addr_size = sizeof(local_addr);
	if (getsockname(sockfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0) {
		asprintf(&log, "failed to get local address information for socket: %d", sockfd);
		PRINT_ERR_FREE(log);
	}

	asprintf(&log, "Sync connection: local:%d [socket:%d] --> %s:%d",
		ntohs(test->domain == AF_INET?
					((struct sockaddr_in *)&local_addr)->sin_port:
					((struct sockaddr_in6 *)&local_addr)->sin6_port),
		sockfd,	ip_address_str, sync_port);
	PRINT_DBG_FREE(log);
	free(ip_address_str);
	freeaddrinfo(serv_info);

	return sockfd;
}

/* check if receiver is busy. if return:
 * -1: indicates error;
 *  0: receiver is NOT busy;
 *  1: receiver is busy.
 */
int query_receiver_busy_state(int sockfd)
{
	int request = (int)'X'; //the int to be sent
	int converted = htonl(request);
	int response = 0; //the int to be received

	if ( write(sockfd, &converted, sizeof(converted)) < 0 ) {
		PRINT_ERR("cannot write data to the socket for sender/receiver sync");
		return -1;
	}
	if ( read(sockfd, &response, sizeof(response)) <= 0 ) {
		PRINT_ERR("cannot read data from the socket for sender/receiver sync");
		return -1;
	}

	if (ntohl(response) == TEST_RUNNING) {
		PRINT_ERR("receiver is busy with an existing test running");
		return 1;   //server is busy
	}

	return 0;  //server is not busy
}

/* negotiate a test duration time with receiver. if return:
 * -1: indicates error;
 *  Non-Zero positive integer: negotiated test duration, returned from receiver;
 */
int negotiate_test_duration(int sockfd, int proposed_time)
{
	int converted = htonl(proposed_time);
	int response = 0; //the int to be received

	if ( write(sockfd, &converted, sizeof(converted)) < 0 ) {
		PRINT_ERR("cannot write data to the socket for sender/receiver sync");
		return -1;
	}
	if ( read(sockfd, &response, sizeof(response)) <= 0 ) {
		PRINT_ERR("cannot read data from the socket for sender/receiver sync");
		return -1;
	}

	return ntohl(response);
}

/* request server to start the test. if return:
 * -1: indicates error;
 *  0: failed, receiver is not ready;
 *  1: success, start the test immediately;
 */
int request_to_start(int sockfd)
{
	int request = (int)'R'; //the int to be sent
	int converted = htonl(request);
	int response = 0; //the int to be received

	if ( write(sockfd, &converted, sizeof(converted)) < 0 ) {
		PRINT_ERR("cannot write data to the socket for sender/receiver sync");
		return -1;
	}
	if ( read(sockfd, &response, sizeof(response)) <= 0 ) {
		PRINT_ERR("cannot read data from the socket for sender/receiver sync");
		return -1;
	}

	if (ntohl(response) != (int)'R') {
		PRINT_ERR("receiver is not ready to run test");
		return 0;   //server is not ready
	}

	return 1;
}

/************************************************************/
//		ntttcp receiver sync functions
/************************************************************/
void *create_receiver_sync_socket( void *ptr )
{
	char *log = NULL;
	struct ntttcp_test_endpoint *tep = (struct ntttcp_test_endpoint *)ptr;
	struct ntttcp_test *test = tep->test;
	struct ntttcp_stream_server *ss;

	int sync_listener = 0;
	bool verbose_log = test->verbose;

	int answer_to_send = 0;  //the int to be sent
	int converted = 0;
	int request_received = 0; //the int to be received
	int nbytes;

	int n_fds = 0, newfd, current_fd = 0;
	fd_set read_set, write_set;

	struct sockaddr_storage peer_addr, local_addr; //for remote peer, and local address
	socklen_t peer_addr_size, local_addr_size;
	struct addrinfo hints, *serv_info;
	char *ip_address_str;
	char *port_str;
	int ip_address_max_size;

	ss = new_ntttcp_server_stream(test);
	if (ss == NULL) {
		PRINT_ERR("receiver: error when creating new server stream");
		return NULL;
	}
	ss->server_port = test->server_base_port - 1;
	ss->protocol = TCP; //no matter what test will be executed, the synch thread always uses TCP
	ss->listener = ntttcp_server_listen(ss);
	if (sync_listener == -1) {
		PRINT_ERR("receiver: failed to listen on sync port");
		return NULL;
	}

	/* bind to local address */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = test->domain;
	hints.ai_socktype = TCP;  //sync, only use TCP protocol
	asprintf(&port_str, "%d", ss->server_port);
	if (getaddrinfo(test->bind_address, port_str, &hints, &serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return NULL;
	}
	free(port_str);

	ip_address_max_size = (test->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return NULL;
	}

	ip_address_max_size = (ss->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address of peer");
		return NULL;
	}

	/* accept new client, receive data from client */
	while (1) {
		memcpy(&read_set, &ss->read_set, sizeof(fd_set));
		memcpy(&write_set, &ss->write_set, sizeof(fd_set));

		/* we are notified by select() */
		n_fds = select(ss->max_fd + 1, &read_set, NULL, NULL, NULL);
		if (n_fds < 0 && errno != EINTR) {
			PRINT_ERR("error happened when select()");
			continue;
		}

		/*run through the existing connections looking for data to be read*/
		for (current_fd = 0; current_fd <= ss->max_fd; current_fd++){
			if ( !FD_ISSET(current_fd, &read_set) )
				continue;

			/* then, we got one fd to hanle */
			/* a NEW connection coming */
			if (current_fd == ss->listener) {
 				/* handle new connections */
				peer_addr_size = sizeof(peer_addr);
				if ((newfd = accept(ss->listener, (struct sockaddr *) &peer_addr, &peer_addr_size)) < 0 ) {
					break;
				}

				/* then we got a new connection */
				if (set_socket_non_blocking(newfd) == -1) {
					asprintf(&log, "cannot set the new socket as non-blocking: %d", newfd);
					PRINT_DBG_FREE(log);
				}
				FD_SET(newfd, &ss->read_set); /* add the new one to read_set */
				if (newfd > ss->max_fd) {
					/* update the maximum */
					ss->max_fd = newfd;
				}

				/* print out new connection info */
				local_addr_size = sizeof(local_addr);
				if (getsockname(newfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0) {
					asprintf(&log, "failed to get local address information for the new socket: %d", newfd);
					PRINT_DBG_FREE(log);
				}
				else{
					asprintf(&log, "Sync connection: %s:%d --> local:%d [socket %d]",
							ip_address_str = retrive_ip_address_str(&peer_addr, ip_address_str, ip_address_max_size),
							ntohs( test->domain == AF_INET ?
									((struct sockaddr_in *)&peer_addr)->sin_port
									:((struct sockaddr_in6 *)&peer_addr)->sin6_port),
							ntohs( test->domain == AF_INET ?
									((struct sockaddr_in *)&local_addr)->sin_port
									:((struct sockaddr_in6 *)&local_addr)->sin6_port),
							newfd);
					PRINT_DBG_FREE(log);
				}
			}
			/* handle data from an EXISTING client */
			else{
				/* got error or connection closed by client */
				if ((nbytes = read(current_fd, &request_received, sizeof(request_received))) <= 0) {
					if (nbytes == 0) {
						asprintf(&log, "socket closed: %d", current_fd);
						PRINT_DBG_FREE(log);
					}
					else{
						asprintf(&log, "error: cannot read data from socket: %d", current_fd);
						PRINT_INFO_FREE(log);
					}
					close(current_fd);
					FD_CLR(current_fd, &ss->read_set); /* remove from master set when finished */
				}
				/* reply sender's sync request */
				else{
					converted = ntohl(request_received);

					switch (converted) {
					case (int)'X':  //query state
						answer_to_send = tep->state;
						break;

					case (int)'R':  //request to start test
						answer_to_send = (int)'R';
						tep->state = TEST_RUNNING;

						turn_on_light();
						PRINT_INFO("Network activity progressing...");
						break;

					default:  //negotiate test duration, use the max one
						if (tep->test->duration < converted) {
							answer_to_send = converted;
							tep->confirmed_duration = answer_to_send;

							asprintf(&log, "test duration negotiated is: %d seconds", answer_to_send);
							PRINT_INFO_FREE(log);
						}
						else {
							answer_to_send = tep->test->duration;
							tep->confirmed_duration = answer_to_send;
						}
					}

					converted = htonl(answer_to_send);
					nbytes = write(current_fd, &converted, sizeof(converted));
					if (nbytes < 0) {
						PRINT_ERR("cannot write ack data to the socket for sender/receiver sync");
					}
				}
			}
		}
	}

	free(ip_address_str);
	close(ss->listener);
	free(ss);
	return NULL;
}
