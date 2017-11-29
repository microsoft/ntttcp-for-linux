// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "tcpstream.h"

/************************************************************/
//		ntttcp socket functions
/************************************************************/
int n_read(int fd, char *buffer, size_t total)
{
	register ssize_t rtn;
	register size_t left = total;

	while (left > 0) {
		rtn = read(fd, buffer, left);
		if (rtn < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				break;
			} else {
				printf("socket read error: %d\n", errno);
				return ERROR_NETWORK_READ;
			}
		}
		else if (rtn == 0)
			break;

		left -= rtn;
		buffer += rtn;
	}

	return total - left;
}

int n_write(int fd, const char *buffer, size_t total)
{
	register ssize_t rtn;
	register size_t left = total;

	while (left > 0) {
		rtn = write(fd, buffer, left);
		if (rtn < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				return total - left;
			} else {
				printf("socket write error: %d\n", errno);
				return ERROR_NETWORK_WRITE;
			}
		}
		else if (rtn == 0)
			return ERROR_NETWORK_WRITE;

		left -= rtn;
		buffer += rtn;
	}
	return total;
}

/************************************************************/
//		ntttcp sender
/************************************************************/
void *run_ntttcp_sender_tcp_stream( void *ptr )
{
	char *log        = NULL;
	bool verbose_log = false;
	int sockfd       = 0; //socket id
	char *buffer;         //send buffer
	int n            = 0; //write n bytes to socket
	uint64_t nbytes  = 0; //total bytes sent
	int i            = 0; //hold function return value
	struct ntttcp_stream_client *sc;

	struct sockaddr_storage local_addr; //for local address
	socklen_t local_addr_size;          //local address size

	char *remote_addr_str = NULL; //used to get remote peer's ip address
	int ip_addr_max_size;  //used to get remote peer's ip address
	char *port_str;        //used to get remote peer's port number
	struct addrinfo hints, *remote_serv_info, *p; //to get remote peer's sockaddr

	struct timeval timeout = {5, 0}; //set socket timeout

	sc = (struct ntttcp_stream_client *) ptr;
	verbose_log = sc->verbose;

	/* get address of remote receiver */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = sc->domain;
	hints.ai_socktype = sc->protocol;
	ASPRINTF(&port_str, "%d", sc->server_port);
	if (getaddrinfo(sc->bind_address, port_str, &hints, &remote_serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return 0;
	}
	free(port_str);

	/* only get the first entry if connected */
	for (p = remote_serv_info; p != NULL; p = p->ai_next) {
		/* 1. create socket fd */
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			PRINT_ERR("cannot create socket ednpoint");
			freeaddrinfo(remote_serv_info);
			return 0;
		}
		else{
		/* 1a. set socket timeout */
			if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
				ASPRINTF(&log, "cannot set socket options: %d", sockfd);
				PRINT_ERR_FREE(log);
				freeaddrinfo(remote_serv_info);
				close(sockfd);
				return 0;
			}
			if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
				ASPRINTF(&log, "cannot set socket options: %d", sockfd);
				PRINT_ERR_FREE(log);
				freeaddrinfo(remote_serv_info);
				close(sockfd);
				return 0;
			}
		}

		local_addr_size = sizeof(local_addr);

		/* 2. bind this socket fd to a local (random/ephemeral, or fixed) TCP port */
		if (sc->domain == AF_INET) {
			(*(struct sockaddr_in*)&local_addr).sin_family = AF_INET;
			(*(struct sockaddr_in*)&local_addr).sin_port = htons(sc->client_port);
		}
		else{
			(*(struct sockaddr_in6*)&local_addr).sin6_family = AF_INET6;
			(*(struct sockaddr_in6*)&local_addr).sin6_port = htons(sc->client_port);
		}

		if (( i = bind(sockfd, (struct sockaddr *)&local_addr, local_addr_size)) < 0 ) {
			ASPRINTF(&log,
				"failed to bind socket: %d to a local port: %d. errno = %d",
				sockfd,
				sc->client_port,
				errno);
			PRINT_ERR_FREE(log);
		}

		/* 3. connect to receiver */
		ip_addr_max_size = (sc->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
		remote_addr_str = malloc(ip_addr_max_size);
		if (remote_addr_str == NULL) {
			PRINT_ERR("cannot allocate memory for ip address string");
			freeaddrinfo(remote_serv_info);
			return 0;
		}
		remote_addr_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr,
							remote_addr_str,
							ip_addr_max_size);
		if (( i = connect(sockfd, p->ai_addr, p->ai_addrlen)) < 0) {
			if (i == -1) {
				ASPRINTF(&log,
					"failed to connect to receiver: %s:%d on socket: %d. errno = %d",
					remote_addr_str,
					sc->server_port,
					sockfd,
					errno);
				PRINT_ERR_FREE(log);
			}
			else {
				ASPRINTF(&log,
					"failed to connect to receiver: %s:%d on socket: %d. error code = %d",
					remote_addr_str,
					sc->server_port,
					sockfd,
					i);
				PRINT_ERR_FREE(log);
			}
			freeaddrinfo(remote_serv_info);
			free(remote_addr_str);
			close(sockfd);
			return 0;
		}
		else{
			break; //connected
		}
	}

	/* get local TCP ephemeral port number assigned, for logging */
	if (getsockname(sockfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0) {
		ASPRINTF(&log,
			"failed to get local address information for socket: %d",
			sockfd);
		PRINT_ERR_FREE(log);
	}

	ASPRINTF(&log, "New connection: local:%d [socket:%d] --> %s:%d",
			ntohs(sc->domain == AF_INET?
					((struct sockaddr_in *)&local_addr)->sin_port:
					((struct sockaddr_in6 *)&local_addr)->sin6_port),
			sockfd,
			remote_addr_str,
			sc->server_port);
	PRINT_DBG_FREE(log);
	free(remote_addr_str);
	freeaddrinfo(remote_serv_info);

	/* wait for sync thread to finish */
	wait_light_on();

	if ((buffer = (char *)malloc(sc->send_buf_size * sizeof(char))) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for send buffer");
		close(sockfd);
		return 0;
	}
	//fill_buffer(buffer, sc->send_buf_size);
	memset(buffer, 'A', sc->send_buf_size * sizeof(char));

	while ( is_light_turned_on(sc->continuous_mode) ) {
		n = n_write(sockfd, buffer, strlen(buffer));
		if (n < 0) {
			PRINT_ERR("cannot write data to a socket");
			free(buffer);
			close(sockfd);
			return 0;
		}
		nbytes += n;
	}
	sc->total_bytes_transferred = nbytes;
	free(buffer);
	close(sockfd);

	return 0;
}

/************************************************************/
//		ntttcp receiver
/************************************************************/
/* listen on the port specified by ss, and return the socket fd */
int ntttcp_server_listen(struct ntttcp_stream_server *ss)
{
	char *log;
	bool verbose_log = ss->verbose;
	int i            = 0; //hold function return value
	int opt          = 1;
	int sockfd       = 0;  //socket file descriptor
	char *local_addr_str;  //used to get local ip address
	int ip_addr_max_size;  //used to get local ip address
	char *port_str;        //used to get port number string for getaddrinfo()
	struct addrinfo hints, *serv_info, *p; //to get local sockaddr for bind()

	/* get receiver/itself address */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = ss->domain;
	hints.ai_socktype = ss->protocol;
	ASPRINTF(&port_str, "%d", ss->server_port);
	if (getaddrinfo(ss->bind_address, port_str, &hints, &serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return -1;
	}
	free(port_str);

	ip_addr_max_size = (ss->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (local_addr_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return -1;
	}

	/* get the first entry to bind and listen */
	for (p = serv_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			PRINT_ERR("cannot create socket ednpoint");
			freeaddrinfo(serv_info);
			free(local_addr_str);
			return -1;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
			ASPRINTF(&log, "cannot set socket options: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(local_addr_str);
			close(sockfd);
			return -1;
		}
		if ( set_socket_non_blocking(sockfd) == -1) {
			ASPRINTF(&log, "cannot set socket as non-blocking: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(local_addr_str);
			close(sockfd);
			return -1;
		}
		if (( i = bind(sockfd, p->ai_addr, p->ai_addrlen)) < 0) {
			ASPRINTF(&log,
				"failed to bind the socket to local address: %s on socket: %d. return = %d",
				local_addr_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr,
									local_addr_str,
									ip_addr_max_size),
				sockfd,
				i );

			if (i == -1) //append more info to log
				ASPRINTF(&log, "%s. errcode = %d", log, errno);
			PRINT_DBG_FREE(log);
			continue;
		}
		else{
			break; //connected
		}
	}
	freeaddrinfo(serv_info);
	free(local_addr_str);
	if (p == NULL) {
		ASPRINTF(&log, "cannot bind the socket on address: %s", ss->bind_address);
		PRINT_ERR_FREE(log);
		close(sockfd);
		return -1;
	}

	ss->listener = sockfd;
	if (listen(ss->listener, MAX_CONNECTIONS_PER_THREAD) < 0) {
		ASPRINTF(&log, "failed to listen on address: %s: %d", ss->bind_address, ss->server_port);
		PRINT_ERR_FREE(log);
		close(ss->listener);
		return -1;
	}

	FD_ZERO(&ss->read_set);
	FD_ZERO(&ss->write_set);
	FD_SET(ss->listener, &ss->read_set);
	if (ss->listener > ss->max_fd)
		ss->max_fd = ss->listener;

	ASPRINTF(&log, "ntttcp server is listening on %s:%d", ss->bind_address, ss->server_port);
	PRINT_DBG_FREE(log);

	return ss->listener;
}

int ntttcp_server_epoll(struct ntttcp_stream_server *ss)
{
	int err_code = NO_ERROR;
	char *log    = NULL;
	bool verbose_log = ss->verbose;

	int efd = 0, n_fds = 0, newfd = 0, current_fd = 0;
	char *buffer;  //receive buffer
	uint64_t nbytes;   //bytes read
	int bytes_to_be_read = 0;  //read bytes from socket
	struct epoll_event event, *events;

	struct sockaddr_storage peer_addr, local_addr; //for remote peer, and local address
	socklen_t peer_addr_size, local_addr_size;
	char *ip_address_str;
	int ip_addr_max_size;
	int i = 0;

	if ( (buffer = (char *)malloc(ss->recv_buf_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for receive buffer");
		return ERROR_MEMORY_ALLOC;
	}
	ip_addr_max_size = (ss->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address of peer");
		free(buffer);
		return ERROR_MEMORY_ALLOC;
	}

	efd = epoll_create1 (0);
	if (efd == -1) {
		PRINT_ERR("epoll_create1 failed");
		free(buffer);
		free(ip_address_str);
		return ERROR_EPOLL;
	}

	event.data.fd = ss->listener;
	event.events = EPOLLIN;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, ss->listener, &event) != 0) {
		PRINT_ERR("epoll_ctl failed");
		free(buffer);
		free(ip_address_str);
		close(efd);
		return ERROR_EPOLL;
	}

	/* Buffer where events are returned */
	events = calloc (MAX_EPOLL_EVENTS, sizeof event);

	while (1) {
		n_fds = epoll_wait (efd, events, MAX_EPOLL_EVENTS, -1);
		for (i = 0; i < n_fds; i++) {
			current_fd = events[i].data.fd;

			if ((events[i].events & EPOLLERR) ||
			    (events[i].events & EPOLLHUP) ||
			    (!(events[i].events & EPOLLIN))) {
				/* An error has occurred on this fd, or the socket is not ready for reading */
				PRINT_ERR("error happened on the associated connection");
				close (current_fd);
				continue;
			}

			/* then, we got one fd to hanle */
			/* a NEW connection coming */
			if (current_fd == ss->listener) {
				/* We have a notification on the listening socket, which means one or more incoming connections. */
				while (1) {
					peer_addr_size = sizeof (peer_addr);
					newfd = accept (ss->listener, (struct sockaddr *) &peer_addr, &peer_addr_size);
					if (newfd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							/* We have processed all incoming connections. */
							break;
						}
						else {
							ASPRINTF(&log, "error to accept new connections. errno = %d", errno)
							PRINT_ERR_FREE(log);
							break;
						}
					}
					if (set_socket_non_blocking(newfd) == -1) {
						ASPRINTF(&log, "cannot set the new socket as non-blocking: %d", newfd);
						PRINT_DBG_FREE(log);
					}

					local_addr_size = sizeof(local_addr);
					if (getsockname(newfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0) {
						ASPRINTF(&log, "failed to get local address information for the new socket: %d", newfd);
						PRINT_DBG_FREE(log);
					}
					else {
						ASPRINTF(&log, "New connection: %s:%d --> local:%d [socket %d]",
								ip_address_str = retrive_ip_address_str(&peer_addr, ip_address_str, ip_addr_max_size),
								ntohs( ss->domain == AF_INET ?
										((struct sockaddr_in *)&peer_addr)->sin_port
										:((struct sockaddr_in6 *)&peer_addr)->sin6_port),
								ntohs( ss->domain == AF_INET ?
										((struct sockaddr_in *)&local_addr)->sin_port
										:((struct sockaddr_in6 *)&local_addr)->sin6_port),
								newfd);
						PRINT_DBG_FREE(log);
					}

					event.data.fd = newfd;
					event.events = EPOLLIN;
					if (epoll_ctl (efd, EPOLL_CTL_ADD, newfd, &event) != 0)
						PRINT_ERR("epoll_ctl failed");

					//if there is no synch thread, if any new connection coming, indicates ss started
					if ( ss->no_synch )
						turn_on_light();
					//else, leave the sync thread to fire the trigger
				}
			}
			/* handle data from an EXISTING client */
			else {
				bzero(buffer, ss->recv_buf_size);
				bytes_to_be_read = ss->is_sync_thread ? 1 : ss->recv_buf_size;

				/* got error or connection closed by client */
				if ((nbytes = n_read(current_fd, buffer, bytes_to_be_read)) <= 0) {
					if (nbytes == 0) {
						ASPRINTF(&log, "socket closed: %d", i);
						PRINT_DBG_FREE(log);
					}
					else {
						ASPRINTF(&log, "error: cannot read data from socket: %d", i);
						PRINT_INFO_FREE(log);
						err_code = ERROR_NETWORK_READ;
						/* need to continue ss and check other socket, so don't end the ss */
					}
					close (current_fd);
				}
				/* report how many bytes received */
				else {
					__sync_fetch_and_add(&(ss->total_bytes_transferred), nbytes);
				}
			}
		}
	}

	free(buffer);
	free(ip_address_str);
	free(events);
	close(efd);
	close(ss->listener);
	return err_code;
}

int ntttcp_server_select(struct ntttcp_stream_server *ss)
{
	int err_code = NO_ERROR;
	char *log    = NULL;
	bool verbose_log = ss->verbose;

	int n_fds = 0, newfd, current_fd = 0;
	char *buffer;  //receive buffer
	uint64_t nbytes;   //bytes read
	int bytes_to_be_read = 0;  //read bytes from socket
	fd_set read_set, write_set;

	struct sockaddr_storage peer_addr, local_addr; //for remote peer, and local address
	socklen_t peer_addr_size, local_addr_size;
	char *ip_address_str;
	int ip_addr_max_size;

	if ( (buffer = (char *)malloc(ss->recv_buf_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for receive buffer");
		return ERROR_MEMORY_ALLOC;
	}
	ip_addr_max_size = (ss->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address of peer");
		free(buffer);
		return ERROR_MEMORY_ALLOC;
	}

	/* accept new client, receive data from client */
	while (1) {
		memcpy(&read_set, &ss->read_set, sizeof(fd_set));
		memcpy(&write_set, &ss->write_set, sizeof(fd_set));

		/* we are notified by select() */
		n_fds = select(ss->max_fd + 1, &read_set, NULL, NULL, NULL);
		if (n_fds < 0 && errno != EINTR) {
			PRINT_ERR("error happened when select()");
			err_code = ERROR_SELECT;
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
					err_code = ERROR_ACCEPT;
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
					ASPRINTF(&log, "New connection: %s:%d --> local:%d [socket %d]",
							ip_address_str = retrive_ip_address_str(&peer_addr, ip_address_str, ip_addr_max_size),
							ntohs( ss->domain == AF_INET ?
									((struct sockaddr_in *)&peer_addr)->sin_port
									:((struct sockaddr_in6 *)&peer_addr)->sin6_port),
							ntohs( ss->domain == AF_INET ?
									((struct sockaddr_in *)&local_addr)->sin_port
									:((struct sockaddr_in6 *)&local_addr)->sin6_port),
							newfd);
					PRINT_DBG_FREE(log);
				}

				//if there is no synch thread, if any new connection coming, indicates ss started
				if ( ss->no_synch )
					turn_on_light();
				//else, leave the sync thread to fire the trigger
			}
			/* handle data from an EXISTING client */
			else{
				bzero(buffer, ss->recv_buf_size);
				bytes_to_be_read = ss->is_sync_thread ? 1 : ss->recv_buf_size;

				/* got error or connection closed by client */
				if ((nbytes = n_read(current_fd, buffer, bytes_to_be_read)) <= 0) {
					if (nbytes == 0) {
						ASPRINTF(&log, "socket closed: %d", current_fd);
						PRINT_DBG_FREE(log);
					}
					else{
						ASPRINTF(&log, "error: cannot read data from socket: %d", current_fd);
						PRINT_INFO_FREE(log);
						err_code = ERROR_NETWORK_READ;
						/* need to continue test and check other socket, so don't end the test */
					}
					close(current_fd);
					FD_CLR(current_fd, &ss->read_set); /* remove from master set when finished */
				}
				/* report how many bytes received */
				else{
					__sync_fetch_and_add(&(ss->total_bytes_transferred), nbytes);
				}
			}
		}
	}

	free(buffer);
	free(ip_address_str);
	close(ss->listener);
	return err_code;
}

void *run_ntttcp_receiver_tcp_stream( void *ptr )
{
	char *log = NULL;
	struct ntttcp_stream_server *ss;

	ss = (struct ntttcp_stream_server *) ptr;

	ss->listener = ntttcp_server_listen(ss);
	if (ss->listener < 0) {
		ASPRINTF(&log, "listen error at port: %d", ss->server_port);
		PRINT_ERR_FREE(log);
	}
	else{
		if (ss->use_epoll == true) {
			if ( ntttcp_server_epoll(ss) != NO_ERROR ) {
				ASPRINTF(&log, "epoll error at port: %d", ss->server_port);
				PRINT_ERR_FREE(log);
			}
		}
		else {
			if ( ntttcp_server_select(ss) != NO_ERROR ) {
				ASPRINTF(&log, "select error at port: %d", ss->server_port);
				PRINT_ERR_FREE(log);
			}
		}
	}

	return NULL;
}

