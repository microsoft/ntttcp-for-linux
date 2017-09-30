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
	hints.ai_socktype = TCP;
	ASPRINTF(&port_str, "%d", sync_port);
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
				ASPRINTF(&log, "failed to connect to receiver: %s:%d on socket: %d. errno = %d", ip_address_str, sync_port, sockfd, errno);
				PRINT_ERR_FREE(log);
			}
			else {
				ASPRINTF(&log, "failed to connect to receiver: %s:%d on socket: %d. error code = %d", ip_address_str, sync_port, sockfd, i);
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
		ASPRINTF(&log, "failed to get local address information for socket: %d", sockfd);
		PRINT_ERR_FREE(log);
	}

	ASPRINTF(&log, "Sync connection: local:%d [socket:%d] --> %s:%d",
		ntohs(test->domain == AF_INET?
					((struct sockaddr_in *)&local_addr)->sin_port:
					((struct sockaddr_in6 *)&local_addr)->sin6_port),
		sockfd,	ip_address_str, sync_port);
	PRINT_DBG_FREE(log);
	free(ip_address_str);
	freeaddrinfo(serv_info);

	return sockfd;
}

/* tell receiver that sender test exited */
void tell_receiver_test_exit(int sockfd)
{
	int request = (int)'E'; //the int to be sent
	int converted = htonl(request);
	int response = 0; //the int to be received

	if ( write(sockfd, &converted, sizeof(converted)) < 0 ) {
		PRINT_ERR("cannot write data to the socket for sender/receiver sync");
	}
	if ( read(sockfd, &response, sizeof(response)) <= 0 ) {
		PRINT_ERR("cannot read data from the socket for sender/receiver sync");
	}

	if (ntohl(response) == TEST_INTERRUPTED || ntohl(response) == TEST_FINISHED) {
		PRINT_INFO("receiver exited from current test");
	} else {
		PRINT_ERR("receiver is not able to handle this interrupt");
	}
}

/* check if receiver is busy. if receiver returns:
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

/* negotiate a test duration time with receiver. if receiver returns:
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

/* request server to start the test. if receiver returns:
 * -1: indicates error;
 *  0: failed, receiver is not ready;
 *  1: success, start the test immediately;
 */
int request_to_start(int sockfd, int request)
{
	/*
	 * the 'request' can be either one of below:
	 * 1) 'R': request receiver to start running the test;
	 * 1) 'L': I am the last client and request receiver to start running the test with all clients.
	 */
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
	if (ntohl(response) == (int)'W') {
		PRINT_INFO ("waiting for the last client to join the test");
		if ( read(sockfd, &response, sizeof(response)) <= 0 ) {
			PRINT_ERR("cannot read data from the socket for sender/receiver sync");
			return -1;
		}
	}
	if (ntohl(response) != (int)'R') {
		PRINT_ERR("receiver is not ready to run test with this sender");
		return 0;   //server is not ready
	}

	return 1;
}

void reply_sender( int fd, int answer_to_send )
{
	int converted = 0;
	int nbytes;

	converted = htonl(answer_to_send);
	nbytes = write(fd, &converted, sizeof(converted));
	if (nbytes < 0) {
		PRINT_ERR("cannot write ack data to the socket for sender/receiver sync");
	}
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

	int i = 0;

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
	ASPRINTF(&port_str, "%d", ss->server_port);
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
					ASPRINTF(&log, "cannot set the new socket as non-blocking: %d", newfd);
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
					ASPRINTF(&log, "failed to get local address information for the new socket: %d", newfd);
					PRINT_DBG_FREE(log);
				}
				else{
					ASPRINTF(&log, "Sync connection: %s:%d --> local:%d [socket %d]",
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
						ASPRINTF(&log, "socket closed: %d", current_fd);
						PRINT_DBG_FREE(log);
					}
					else{
						ASPRINTF(&log, "error: cannot read data from socket: %d", current_fd);
						PRINT_ERR_FREE(log);
					}
					close(current_fd);
					FD_CLR(current_fd, &ss->read_set); /* remove from master set when finished */
				}
				/* reply sender's sync request */
				else{
					converted = ntohl(request_received);

					switch (converted) {
					case (int)'E':  //Exit current test
						if (tep->state == TEST_RUNNING){
							turn_off_light();
							tep->state = TEST_INTERRUPTED;
							ASPRINTF(&log, "test exited because sender side was interrupted");
							PRINT_INFO_FREE(log);
						}
						answer_to_send = tep->state;
						break;

					case (int)'X':  //query state
						answer_to_send = tep->state;
						break;

					case (int)'R':  //request to start test
						if (tep->test->multi_clients_mode == true) {
							if (tep->num_remote_endpoints >= MAX_REMOTE_ENDPOINTS - 1) {
								/* this client wants to seat at the last position;
								 * but, the last position is reserved for the client with '-L' flag;
								 * so, reject this client.
								 */
								PRINT_ERR("too many client endpoints. reject one.");
								answer_to_send = (int)'E'; //'E': ERROR
							} else {
								answer_to_send = (int)'W'; //'W': PLEASE WAIT
								tep->remote_endpoints[ tep->num_remote_endpoints++ ] = current_fd;
							}
						} else {
							answer_to_send = (int)'R'; //'R': RUN
							tep->state = TEST_RUNNING;

							turn_on_light();
							PRINT_INFO("Network activity progressing...");
						}
						break;

					case (int)'L':  //the last client joined, and request to start the test
						if (tep->test->multi_clients_mode == true) {
							if (tep->num_remote_endpoints >= MAX_REMOTE_ENDPOINTS) {
								/* the last seat has been taken; too many client! */
								answer_to_send = (int)'E'; //'E': ERROR
							}

							/* firstly, add this client into the client collection */
							tep->remote_endpoints[ tep->num_remote_endpoints++ ] = current_fd;

							/* notify all clients to run! */
							answer_to_send = (int)'R';
							for (i = 0; i < MAX_REMOTE_ENDPOINTS; i++)
								if (tep->remote_endpoints[i] != -1)
									reply_sender(tep->remote_endpoints[i], answer_to_send);

							tep->state = TEST_RUNNING;
							turn_on_light();
							PRINT_INFO("Network activity progressing...");
						} else {
							PRINT_ERR("one sender endpoint says it is the last client ('-L');");
							PRINT_ERR("but receiver currently is not running in multi-clients mode ('-M');");
							PRINT_ERR("this sender endpoint will be ignored.");
						}
						break;

					default:
						if (tep->test->multi_clients_mode == true) {
							/* in multi-clients mode,
							 * all sender clients use the test duration specified by receiver.
							 */
							answer_to_send = tep->test->duration;
							tep->confirmed_duration = answer_to_send;
						} else if (converted == 0) {
							/* the sender request to run with "continuous_mode" (duration == 0),
							 * receiver then will accept that mode.
							 */
							if (tep->test->duration !=0)
								PRINT_INFO("test is negotiated to run with continuous mode");
							answer_to_send = 0;
							tep->confirmed_duration = 0;
						} else {
							/* if receiver is specified to run with "continuous_mode", then tell sender to do so;
							 * else, compare and use the max time as negotiated test duration time
							 */
							if (tep->test->duration == 0) {
								//then tell sender to run with "continuous_mode" too
								answer_to_send = 0;
								tep->confirmed_duration = 0;
							} else if (tep->test->duration < converted) {
								answer_to_send = converted;
								tep->confirmed_duration = answer_to_send;

								ASPRINTF(&log, "test duration negotiated is: %d seconds", answer_to_send);
								PRINT_INFO_FREE(log);
							}
							else {
								answer_to_send = tep->test->duration;
								tep->confirmed_duration = answer_to_send;
							}
						}
					}

					reply_sender(current_fd, answer_to_send);
				}
			}
		}
	}

	free(ip_address_str);
	close(ss->listener);
	free(ss);
	return NULL;
}
