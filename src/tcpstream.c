// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "tcpstream.h"

#define MAX_IO_PER_POLL 32

/************************************************************/
//		ntttcp socket functions
/************************************************************/
int n_recv(int fd, char *buffer, size_t total)
{
	register ssize_t rtn;
	register size_t left = total;

	while (left > 0) {
		rtn = recv(fd, buffer, left, 0);
		if (rtn < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				break;
			} else {
				printf("socket read error: %d\n", errno);
				return ERROR_NETWORK_READ;
			}
		} else if (rtn == 0)
			break;

		left -= rtn;
		buffer += rtn;
	}

	return total - left;
}

int n_send(int fd, const char *buffer, size_t total)
{
	register ssize_t rtn;
	register size_t left = total;

	while (left > 0) {
		rtn = send(fd, buffer, left, 0);
		if (rtn < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				return total - left;
			} else {
				/* printf("socket write error: %d\n", errno); */
				return ERROR_NETWORK_WRITE;
			}
		} else if (rtn == 0)
			return ERROR_NETWORK_WRITE;

		left -= rtn;
		buffer += rtn;
	}
	return total;
}

/************************************************************/
//		ntttcp sender
/************************************************************/
void *run_ntttcp_sender_tcp_stream(void *ptr)
{
	char *log = NULL;
	int sockfd = 0; /* socket id */
	uint i = 0; /* for loop iterator */
	char *buffer; /* send buffer */
	int n = 0; /* write n bytes to socket */
	int ret = 0; /* hold function return value */
	uint total_sub_conn_created = 0; /* track how many sub connections created in this thread */
	struct ntttcp_stream_client *sc;

	uint client_port = 0;
	int sockfds[MAX_CLIENT_CONNS_PER_THREAD] = {-1};
	struct sockaddr_storage local_addr; /* for local address */
	socklen_t local_addr_size = sizeof(local_addr); /* local address size */

	char *remote_addr_str = NULL; /* used to get remote peer's ip address */
	int ip_addr_max_size; /* used to get remote peer's ip address */
	char *port_str; /* used to get remote peer's port number */
	struct addrinfo hints, *remote_serv_info, *p; /* to get remote peer's sockaddr */

	struct timeval timeout = {SOCKET_TIMEOUT_SEC, 0}; /* set socket timeout */
	/* the variables below are used to retrieve RTT and calculate average RTT */
	unsigned int total_rtt = 0;
	uint num_average_rtt = 0;
	struct tcp_info tcpinfo;
	uint bytes = sizeof(tcpinfo);

	sc = (struct ntttcp_stream_client *)ptr;

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

	ip_addr_max_size = (sc->domain == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	remote_addr_str = malloc(ip_addr_max_size);
	if (remote_addr_str == NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(remote_serv_info);
		return 0;
	}

	for (i = 0; i < sc->num_connections; i++) {

		/* only get the first entry if connected */
		for (p = remote_serv_info; p != NULL; p = p->ai_next) {
			/* 1. create socket fd */
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
				PRINT_ERR("cannot create a socket endpoint");
				sockfds[i] = -1;
				continue;
				;
			} else {
				/* 1a. set socket timeout */
				if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
					ASPRINTF(&log, "cannot set option SO_SNDTIMEO for socket[%d]", sockfd);
					PRINT_INFO_FREE(log);
					close(sockfd);
					sockfds[i] = -1;
					continue;
				}
				if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
					ASPRINTF(&log, "cannot set option SO_RCVTIMEO for socket[%d]", sockfd);
					PRINT_INFO_FREE(log);
					close(sockfd);
					sockfds[i] = -1;
					continue;
				}
			}

			if (sc->client_port != 0) {
				/* 2. bind this socket fd to a local fixed TCP port */
				client_port = sc->client_port + i;

				memset(&local_addr, 0, sizeof(local_addr));
				if (sc->domain == AF_INET) {
					(*(struct sockaddr_in *)&local_addr).sin_family = AF_INET; /* local_addrs[i].ss_family = AF_INET; */
					(*(struct sockaddr_in *)&local_addr).sin_port = htons(client_port);
				} else {
					(*(struct sockaddr_in6 *)&local_addr).sin6_family = AF_INET6; /* local_addrs[i].ss_family = AF_INET6; */
					(*(struct sockaddr_in6 *)&local_addr).sin6_port = htons(client_port);
				}

				if ((ret = bind(sockfd, (struct sockaddr *)&local_addr, local_addr_size)) < 0) {
					ASPRINTF(&log,
						"failed to bind socket[%d] to a local port: [%s:%d]. errno = %d. Ignored",
						sockfd,
						sc->domain == AF_INET ? inet_ntoa((*(struct sockaddr_in *)&local_addr).sin_addr) : "::", /* TODO - get the IPv6 addr string */
						client_port, errno);
					PRINT_INFO_FREE(log);
				}
			}

			/* 3. connect to receiver */
			remote_addr_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr, remote_addr_str, ip_addr_max_size);
			if ((ret = connect(sockfd, p->ai_addr, p->ai_addrlen)) < 0) {
				/* ignore the EINPROGRESS error, errno = 115, and try to create new connection */
				if (errno == EINPROGRESS) {
					ASPRINTF(&log,
						"ignore the failure to connect to receiver: %s:%d on socket[%d]. return = %d, errno = %d",
						remote_addr_str, sc->server_port, sockfd, ret, errno);
					PRINT_DBG_FREE(log);
					close(sockfd);
					i--;
					break;
				} else {
					ASPRINTF(&log,
						"failed to connect to receiver: %s:%d on socket[%d]. return = %d, errno = %d",
						remote_addr_str, sc->server_port, sockfd, ret, errno);
					PRINT_INFO_FREE(log);
					close(sockfd);
					sockfds[i] = -1;
					continue;
				}
			}

			/* get the local TCP port number assigned to this socket, for logging purpose */
			memset(&local_addr, 0, sizeof(local_addr));
			if (getsockname(sockfd, (struct sockaddr *)&local_addr, &local_addr_size) != 0) {
				ASPRINTF(&log, "failed to get local address information for socket[%d]", sockfd);
				PRINT_INFO_FREE(log);
			}

			/* set socket rate limit if specified by user */
			if (sc->socket_fq_rate_limit_bytes != 0)
				enable_fq_rate_limit(sc, sockfd);

			ASPRINTF(&log,
				"New connection: local:%d [socket:%d] --> %s:%d",
				ntohs(sc->domain == AF_INET ? ((struct sockaddr_in *)&local_addr)->sin_port : ((struct sockaddr_in6 *)&local_addr)->sin6_port),
				sockfd, remote_addr_str, sc->server_port);
			PRINT_DBG_FREE(log);

			/* CONNECTED! */
			sockfds[i] = sockfd;
			total_sub_conn_created++;

			/* this connection is connected. skip next remote_serv_info */
			break;
		}
	}

	/* so far, we have all sub connections created and connected to remote port */
	free(remote_addr_str);
	freeaddrinfo(remote_serv_info);

	if (total_sub_conn_created == 0)
		goto CLEANUP;
	sc->num_conns_created = total_sub_conn_created;

	/* wait for sync thread to finish */
	wait_light_on();

	size_t buffer_len = sc->send_buf_size * sizeof(char);
	if ((buffer = (char *)malloc(buffer_len)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for send buffer");
		goto CLEANUP;
	}
	/* fill_buffer(buffer, sc->send_buf_size); */
	memset(buffer, 'A', buffer_len);

	while (is_light_turned_on()) {
		if (sc->hold_on)
			continue;

		for (i = 0; i < sc->num_connections; i++) {
			sockfd = sockfds[i];
			/* skip those socket fds ('-1') if failed in creation phase */
			if (sockfd < 0)
				continue;
			n = n_send(sockfd, buffer, buffer_len);
			if (n < 0) {
				continue;
			}
			sc->total_bytes_transferred += n;
		}
	}
	free(buffer);

	for (i = 0; i < sc->num_connections; i++) {
		if (sockfds[i] >= 0) {
			if (getsockopt(sockfds[i], SOL_TCP, TCP_INFO, (void *)&tcpinfo, &bytes) != 0) {
				PRINT_INFO("getsockopt (TCP_INFO) failed");
			} else {
				total_rtt += tcpinfo.tcpi_rtt;
				num_average_rtt++;
			}
		}
	}

	if (num_average_rtt > 0) {
		sc->average_rtt = total_rtt / num_average_rtt;
	}

CLEANUP:
	for (i = 0; i < sc->num_connections; i++)
		if (sockfds[i] >= 0)
			close(sockfds[i]);
	return 0;
}

/************************************************************/
//		ntttcp receiver
/************************************************************/
/* listen on the port specified by ss, and return the socket fd */
int ntttcp_server_listen(struct ntttcp_stream_server *ss)
{
	char *log;
	int i = 0; /* hold function return value */
	int opt = 1;
	int sockfd = 0; /* socket file descriptor */
	char *local_addr_str; /* used to get local ip address */
	int ip_addr_max_size; /* used to get local ip address */
	char *port_str; /* used to get port number string for getaddrinfo() */
	struct addrinfo hints, *serv_info, *p; /* to get local sockaddr for bind() */

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

	ip_addr_max_size = (ss->domain == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ((local_addr_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return -1;
	}

	/* get the first entry to bind and listen */
	for (p = serv_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			PRINT_ERR("cannot create socket endpoint");
			freeaddrinfo(serv_info);
			free(local_addr_str);
			return -1;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
			ASPRINTF(&log, "cannot set socket options: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(local_addr_str);
			close(sockfd);
			return -1;
		}
		/* can't set a nonblocking socket with io_uring */
		if(!ss->use_iouring) {
			if (set_socket_non_blocking(sockfd) == -1) {
				ASPRINTF(&log, "cannot set socket as non-blocking: %d", sockfd);
				PRINT_ERR_FREE(log);
				freeaddrinfo(serv_info);
				free(local_addr_str);
				close(sockfd);
				return -1;
			}
		}
		if ((i = bind(sockfd, p->ai_addr, p->ai_addrlen)) < 0) {
			ASPRINTF(&log,
				"failed to bind the socket to local address: %s on socket: %d. return = %d",
				local_addr_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr, local_addr_str, ip_addr_max_size),
				sockfd, i);

			if (i == -1) /* append more info to log */
				ASPRINTF(&log, "%s. errcode = %d", log, errno);
			PRINT_DBG_FREE(log);
			continue;
		} else {
			break; /* connected */
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
	if (listen(ss->listener, MAX_THREADS_PER_SERVER_PORT) < 0) {
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
	/* unblock the other threads */
        if(ss->stream_server_num == 0){
                pthread_barrier_wait(ss->init_barrier_pt);
        }

	int err_code = NO_ERROR;
	char *log = NULL;

	int efd = 0, n_fds = 0, newfd = 0, current_fd = 0;
	char *buffer; /* receive buffer */
	uint64_t nbytes; /* bytes read */
	int bytes_to_be_read = 0; /* read bytes from socket */
	struct epoll_event event, *events;

	struct sockaddr_storage peer_addr, local_addr; /* for remote peer, and local address */
	socklen_t peer_addr_size, local_addr_size;
	char *ip_address_str;
	int ip_addr_max_size;
	int i = 0;
	int max_io = 0;

	if ((buffer = (char *)malloc(ss->recv_buf_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for receive buffer");
		return ERROR_MEMORY_ALLOC;
	}
	ip_addr_max_size = (ss->domain == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ((ip_address_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address of peer");
		free(buffer);
		return ERROR_MEMORY_ALLOC;
	}

	efd = epoll_create1(0);
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
	events = calloc(MAX_EPOLL_EVENTS, sizeof event);

	while (1) {
		if (ss->endpoint->receiver_exit_after_done &&
		    ss->endpoint->state == TEST_FINISHED)
			break;

		n_fds = epoll_wait(efd, events, MAX_EPOLL_EVENTS, -1);
		for (i = 0; i < n_fds; i++) {
			current_fd = events[i].data.fd;

			if ((events[i].events & EPOLLERR) ||
			    (events[i].events & EPOLLHUP) ||
			    (!(events[i].events & EPOLLIN))) {
				/* An error has occurred on this fd, or the socket is not ready for reading */
				PRINT_ERR("error happened on the associated connection");
				close(current_fd);
				continue;
			}

			/* then, we got one fd to handle */
			/* a NEW connection coming */
			if (current_fd == ss->listener) {
				/* We have a notification on the listening socket, which means one or more incoming connections. */
				while (1) {
					peer_addr_size = sizeof(peer_addr);
					newfd = accept(ss->listener, (struct sockaddr *)&peer_addr, &peer_addr_size);
					if (newfd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							/* We have processed all incoming connections. */
							break;
						} else {
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
					if (getsockname(newfd, (struct sockaddr *)&local_addr, &local_addr_size) != 0) {
						ASPRINTF(&log, "failed to get local address information for the new socket: %d", newfd);
						PRINT_DBG_FREE(log);
					} else {
						ASPRINTF(&log,
							"New connection: %s:%d --> local:%d [socket %d]",
							ip_address_str = retrive_ip_address_str(&peer_addr, ip_address_str, ip_addr_max_size),
							ntohs(ss->domain == AF_INET ?
							     ((struct sockaddr_in *)&peer_addr)->sin_port :
							     ((struct sockaddr_in6 *)&peer_addr)->sin6_port),
							ntohs(ss->domain == AF_INET ?
							     ((struct sockaddr_in *)&local_addr)->sin_port :
							     ((struct sockaddr_in6 *)&local_addr)->sin6_port),
							newfd);
						PRINT_DBG_FREE(log);
					}

					event.data.fd = newfd;
					event.events = EPOLLIN;
					if (epoll_ctl(efd, EPOLL_CTL_ADD, newfd, &event) != 0)
						PRINT_ERR("epoll_ctl failed");

					/* if there is no synch thread, if any new connection coming, indicates ss started */
					if (ss->no_synch)
						turn_on_light();
					/* else, leave the sync thread to fire the trigger */
				}
			}
			/* handle data from an EXISTING client */
			else {
				for (max_io = 0; max_io < MAX_IO_PER_POLL; max_io++) {
					bytes_to_be_read = ss->is_sync_thread ? 1 : ss->recv_buf_size;

					/* got error or connection closed by client */
					errno = 0;
					nbytes = n_recv(current_fd, buffer, bytes_to_be_read);
					if (nbytes <= 0) {
						if (errno != EAGAIN) {
							if (nbytes == 0) {
								ASPRINTF(&log, "socket closed: %d", i);
								PRINT_DBG_FREE(log);
							} else {
								ASPRINTF(&log, "error: cannot read data from socket: %d", i);
								PRINT_INFO_FREE(log);
								err_code = ERROR_NETWORK_READ;
								/* need to continue ss and check other socket, so don't end the ss */
							}
							close(current_fd);
						}
						break;
					}
					/* report how many bytes received */
					else {
						__sync_fetch_and_add(&(ss->total_bytes_transferred), nbytes);
					}
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
	/* unblock the other threads */
        if(ss->stream_server_num == 0){
                pthread_barrier_wait(ss->init_barrier_pt);
        }

	int err_code = NO_ERROR;
	char *log = NULL;

	int n_fds = 0, newfd, current_fd = 0;
	char *buffer; /* receive buffer */
	uint64_t nbytes; /* bytes read */
	int bytes_to_be_read = 0; /* read bytes from socket */
	fd_set read_set, write_set;

	struct sockaddr_storage peer_addr, local_addr; /* for remote peer, and local address */
	socklen_t peer_addr_size, local_addr_size;
	char *ip_address_str;
	int ip_addr_max_size;
	int max_io = 0;

	if ((buffer = (char *)malloc(ss->recv_buf_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for receive buffer");
		return ERROR_MEMORY_ALLOC;
	}
	ip_addr_max_size = (ss->domain == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ((ip_address_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address of peer");
		free(buffer);
		return ERROR_MEMORY_ALLOC;
	}

	/* accept new client, receive data from client */
	while (1) {
		if (ss->endpoint->receiver_exit_after_done &&
		    ss->endpoint->state == TEST_FINISHED)
			break;

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
		for (current_fd = 0; current_fd <= ss->max_fd; current_fd++) {
			if (!FD_ISSET(current_fd, &read_set))
				continue;

			/* then, we got one fd to handle */
			/* a NEW connection coming */
			if (current_fd == ss->listener) {
				/* handle new connections */
				peer_addr_size = sizeof(peer_addr);
				if ((newfd = accept(ss->listener, (struct sockaddr *)&peer_addr, &peer_addr_size)) < 0) {
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
				if (getsockname(newfd, (struct sockaddr *)&local_addr, &local_addr_size) != 0) {
					ASPRINTF(&log, "failed to get local address information for the new socket: %d", newfd);
					PRINT_DBG_FREE(log);
				} else {
					ASPRINTF(&log,
						"New connection: %s:%d --> local:%d [socket %d]",
						ip_address_str = retrive_ip_address_str(&peer_addr, ip_address_str, ip_addr_max_size),
						ntohs(ss->domain == AF_INET ? ((struct sockaddr_in *)&peer_addr)->sin_port : ((struct sockaddr_in6 *)&peer_addr)->sin6_port),
						ntohs(ss->domain == AF_INET ? ((struct sockaddr_in *)&local_addr)->sin_port : ((struct sockaddr_in6 *)&local_addr)->sin6_port),
						newfd);
					PRINT_DBG_FREE(log);
				}

				/* if there is no synch thread, if any new connection coming, indicates ss started */
				if (ss->no_synch)
					turn_on_light();
				/* else, leave the sync thread to fire the trigger */
			}
			/* handle data from an EXISTING client */
			else {
				for (max_io = 0; max_io < MAX_IO_PER_POLL; max_io++) {
					bytes_to_be_read = ss->is_sync_thread ? 1 : ss->recv_buf_size;

					/* got error or connection closed by client */
					errno = 0;
					nbytes = n_recv(current_fd, buffer, bytes_to_be_read);
					if (nbytes <= 0) {
						if (errno != EAGAIN) {
							if (nbytes == 0) {
								ASPRINTF(&log, "socket closed: %d", current_fd);
								PRINT_DBG_FREE(log);
							} else {
								ASPRINTF(&log, "error: cannot read data from socket: %d", current_fd);
								PRINT_INFO_FREE(log);
								err_code = ERROR_NETWORK_READ;
								/* need to continue test and check other socket, so don't end the test */
							}
							close(current_fd);
							FD_CLR(current_fd, &ss->read_set); /* remove from master set when finished */
						}
						break;
					}
					/* report how many bytes received */
					else {
						__sync_fetch_and_add(&(ss->total_bytes_transferred), nbytes);
					}
				}
			}
		}
	}

	free(buffer);
	free(ip_address_str);
	close(ss->listener);
	return err_code;
}

int add_accept_request(struct ntttcp_stream_server *ss, struct sockaddr_in *client_addr, socklen_t *client_addr_len) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ss->rings[ss->stream_server_num]);
        io_uring_prep_accept(sqe, ss->listener, (struct sockaddr *) client_addr, client_addr_len, 0);

        io_uring_sqe_set_data(sqe, (void*)(intptr_t)ss->listener);
        //return io_uring_submit(&ss->rings[ss->stream_server_num]);
        return 1;
}

int add_read_request(struct ntttcp_stream_server *ss, int client_socket, const struct iovec *buffer_iov) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ss->rings[ss->stream_server_num]);

        /* Linux kernel 5.5 has support for readv, but not for recv() or read() */
        //io_uring_prep_recv(sqe, client_socket, buffer_iov->iov_base, buffer_iov->iov_len, 0);
        io_uring_prep_read_fixed(sqe, client_socket, buffer_iov->iov_base, buffer_iov->iov_len, 0, 0);

        io_uring_sqe_set_data(sqe, (void*)(intptr_t)client_socket);
        //return io_uring_submit(&ss->rings[ss->stream_server_num]);
        return 1;
}

static int* init_registerfiles(struct ntttcp_stream_server *ss)
{
        struct rlimit r;
        int i, ret;
        static int *files;
        static int *registered_files;

        ret = getrlimit(RLIMIT_NOFILE, &r);
        if (ret < 0) {
                fprintf(stderr, "getrlimit: %s\n", strerror(errno));
                exit(1);
        }

        if (r.rlim_max > 32768)
                r.rlim_max = 32768;

        files = malloc(r.rlim_max * sizeof(int));
        if (!files) {
                fprintf(stderr, "calloc for registered files failed\n");
                exit(1);
        }

        for (i = 0; i < (int)r.rlim_max; i++)
                files[i] = -1;

        registered_files = malloc(r.rlim_max * sizeof(int));
        if (!registered_files) {
                fprintf(stderr, "calloc failed\n");
                exit(1);
        }

        for (i = 0; i < (int)r.rlim_max; i++)
                registered_files[i] = -1;

        ret = io_uring_register_files(&ss->rings[ss->stream_server_num], files, r.rlim_max);
        if (ret < 0) {
                fprintf(stderr, "%s: register %d\n", __FUNCTION__, ret);
                exit(1);
        }
        return registered_files;
}

int ntttcp_server_iouring(struct ntttcp_stream_server *ss)
{
        int err_code = NO_ERROR;

        char *buffer;  //receive buffer
        char *ip_address_str;
        int ip_addr_max_size;

        /* io uring params */
        struct io_uring_cqe *cqe;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

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

        struct iovec buffer_iov = {.iov_base = buffer, .iov_len = ss->recv_buf_size};

        /* can customize params based on flags later  */
        struct io_uring_params p = {};
        int ret;

        if(ss->stream_server_num > 0) {
                p.wq_fd = ss->rings[0].ring_fd; //ss->first_work_queue_fd;
                p.flags |= IORING_SETUP_ATTACH_WQ;

                // initialize io uring queue and return an error if something goes wrong
                ret = io_uring_queue_init_params(QUEUE_DEPTH, &ss->rings[ss->stream_server_num], &p);
        }
        else {
                // initialize io uring queue and return an error if something goes wrong
                ret = io_uring_queue_init_params(QUEUE_DEPTH, &ss->rings[0], &p);

                // unblock the other threads, thread 1 initialization is done
                pthread_barrier_wait(ss->init_barrier_pt);
        }

        if(ret < 0) {
                char* error_msg = strerror(-ret);
                printf("%s\n", error_msg);

                printf("io_uring stream %d init failed...\n", ss->stream_server_num);
                exit(1);
        }


        int *registered_files = init_registerfiles(ss);

        ret = io_uring_register_files_update(&ss->rings[ss->stream_server_num], ss->listener, &ss->listener, 1);
        if (ret < 0) {
                fprintf(stderr, "lege io_uring_register_files_update failed: %d %d\n", ss->listener, ret);
                exit(1);
        }
        registered_files[ss->listener] = ss->listener;


        ret = io_uring_register_buffers(&ss->rings[ss->stream_server_num], &buffer_iov, 1);
        if(ret) {
                fprintf(stderr, "Error registering buffers: %s", strerror(-ret));
                return 1;
        }

        /* accept new client, receive data from client */
        ret = add_accept_request(ss, &client_addr, &client_addr_len);
        io_uring_submit(&ss->rings[ss->stream_server_num]);

        while (1) {
                if (ss->endpoint->receiver_exit_after_done &&
                    ss->endpoint->state == TEST_FINISHED)
                        break;

                /* wait on next request to finish before continuring  */
                ret = io_uring_wait_cqe(&ss->rings[ss->stream_server_num], &cqe);
                int cqe_fd = cqe->user_data;

                if (ret < 0)
                        printf("io_uring wait cqe failed...\n");
                if (cqe->res < 0) {
                        printf("cqe_fd: %d, stream server num: %d, cqe->res: %d\n", cqe_fd, ss->stream_server_num, cqe->res);
                        fprintf(stderr, "Async request failed: %s for event: %d\n",
                                strerror(-cqe->res), cqe_fd);
                                exit(1);
                }

                /* check if the cqe was from a listen or from a read sqe */
                if (cqe_fd == ss->listener) {
                        int sock_conn_fd = cqe->res;

                        if(registered_files[sock_conn_fd] == -1) {
                                ret = io_uring_register_files_update(&ss->rings[ss->stream_server_num], sock_conn_fd, &sock_conn_fd, 1);
                                if (ret < 0) {
                                        fprintf(stderr, "io_uring_register_files_update failed: %d %d\n", sock_conn_fd, ret);
                                        exit(1);
                                 }
                                 registered_files[sock_conn_fd] = sock_conn_fd;
                        }

                        add_accept_request(ss, &client_addr, &client_addr_len);
                        add_read_request(ss, sock_conn_fd, &buffer_iov);

                        //if there is no synch thread, if any new connection coming, indicates ss started
                        if ( ss->no_synch )
                                turn_on_light();
                        //else, leave the sync thread to fire the trigger
                }
                else {
                        int bytes_received = cqe->res;
                        if (!bytes_received) {
                                fprintf(stderr, "Empty request!\n");
                                break;
                        }
                        /* add to the byte count atomically and queue the next request  */
                        __sync_fetch_and_add(&(ss->total_bytes_transferred), bytes_received);
                        add_read_request(ss, cqe_fd, &buffer_iov);
                }

                /* Mark this request as processed */
                io_uring_cqe_seen(&ss->rings[ss->stream_server_num], cqe);
                io_uring_submit(&ss->rings[ss->stream_server_num]);
        }

        free(buffer);
        free(ip_address_str);
        close(ss->listener);
        return err_code;
}

void *run_ntttcp_receiver_tcp_stream(void *ptr)
{
	char *log = NULL;
	struct ntttcp_stream_server *ss;

	ss = (struct ntttcp_stream_server *)ptr;

	ss->listener = ntttcp_server_listen(ss);
	if (ss->listener < 0) {
		ASPRINTF(&log, "listen error at port: %d", ss->server_port);
		PRINT_ERR_FREE(log);
	} else {
		/* Decide on which method to transfer data with (io_uring, epoll, select)  */
		if (ss->use_iouring == true) {
			if ( ntttcp_server_iouring(ss) != NO_ERROR ) {
				ASPRINTF(&log, "select error at port: %d", ss->server_port);
				PRINT_ERR_FREE(log);
			}
		}
		else if (ss->use_epoll == true) {
			if (ntttcp_server_epoll(ss) != NO_ERROR) {
				ASPRINTF(&log, "epoll error at port: %d", ss->server_port);
				PRINT_ERR_FREE(log);
			}
		} else {
			if (ntttcp_server_select(ss) != NO_ERROR) {
				ASPRINTF(&log, "select error at port: %d", ss->server_port);
				PRINT_ERR_FREE(log);
			}
		}
	}

	return NULL;
}
