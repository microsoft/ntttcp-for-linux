// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "ntttcp.h"

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
//		ntttcp multiple threads synch helper
/************************************************************/

static int run_light = 0;
static pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wait_light = PTHREAD_COND_INITIALIZER;

void turn_on_light( void )
{
	pthread_mutex_lock( &light_mutex );
	run_light = 1;
	pthread_cond_broadcast( &wait_light );
	pthread_mutex_unlock( &light_mutex );
}

void turn_off_light( void )
{
	pthread_mutex_lock( &light_mutex );
	run_light = 0;
	pthread_cond_broadcast( &wait_light );
	pthread_mutex_unlock( &light_mutex );
}

void wait_light_on( void )
{
	pthread_mutex_lock( &light_mutex );
	while (run_light == 0)
		pthread_cond_wait( &wait_light, &light_mutex );
	pthread_mutex_unlock( &light_mutex );
}

void wait_light_off( void )
{
        pthread_mutex_lock( &light_mutex );
        while (run_light != 0)
                pthread_cond_wait( &wait_light, &light_mutex );
        pthread_mutex_unlock( &light_mutex );
}

int is_light_turned_on( void )
{
	int temp;

	pthread_mutex_lock( &light_mutex );
	temp = run_light;
	pthread_mutex_unlock( &light_mutex );

	return temp;
}

/************************************************************/
//		ntttcp timer and signal handle
/************************************************************/

void sig_handler(int signo)
{
	//Ctrl+C
	if (signo == SIGINT){
		PRINT_INFO("Interrupted by Ctrl+C");

		if (is_light_turned_on())
			turn_off_light();
		else
			exit (1);
	}
}

void timer_fired()
{
	turn_off_light();
}

void run_test_timer(int duration)
{
	struct itimerval it_val;

	it_val.it_value.tv_sec = duration;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval.tv_sec = 0;
	it_val.it_interval.tv_usec = 0;

	if (signal(SIGALRM, timer_fired) == SIG_ERR) {
		PRINT_ERR("unable to set test timer: signal SIGALRM failed");
		exit(1);
	}
	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
		PRINT_ERR("unable to set test timer: setitimer ITIMER_REAL failed");
		exit(1);
	}
}

/************************************************************/
//		ntttcp sender
/************************************************************/

void *run_ntttcp_sender_stream( void *ptr )
{
	char *log = NULL;
	int sockfd = 0; //socket id
	char *buffer; //send buffer
	struct ntttcp_stream_client *test;
	int n = 0; //write n bytes to socket
	long nbytes = 0;  //total bytes sent

	struct sockaddr_storage local_addr; //for local address
	socklen_t local_addr_size; //local address size, for getsockname(), to get local port
	char *ip_address_str; //used to get remote peer's ip address
	int ip_address_max_size;  //used to get remote peer's ip address
	char *port_str; //to get remote peer's port number for getaddrinfo()
	struct addrinfo hints, *serv_info, *p; //to get remote peer's sockaddr for connect()

	int i = 0;

	test = (struct ntttcp_stream_client *) ptr;

	ip_address_max_size = (test->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL){
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return 0;
	}

	/* connect to remote receiver */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = test->domain;
	hints.ai_socktype = test->protocol;
	asprintf(&port_str, "%d", test->server_port);
	if (getaddrinfo(test->bind_address, port_str, &hints, &serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return 0;
	}
	free(port_str);

	/* only get the first entry to connect */
	for (p = serv_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0){
			PRINT_ERR("cannot create socket ednpoint");
			freeaddrinfo(serv_info);
			free(ip_address_str);
			return 0;
		}
		ip_address_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr, ip_address_str, ip_address_max_size);
		if (( i = connect(sockfd, p->ai_addr, p->ai_addrlen)) < 0){
			if (i == -1){
				asprintf(&log, "failed to connect to receiver: %s:%d on socket: %d. errno = %d", ip_address_str, test->server_port, sockfd, errno);
				PRINT_ERR_FREE(log);
			}
			else {
				asprintf(&log, "failed to connect to receiver: %s:%d on socket: %d. error code = %d", ip_address_str, test->server_port, sockfd, i);
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
	if (getsockname(sockfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0){
		asprintf(&log, "failed to get local address information for socket: %d", sockfd);
		PRINT_ERR_FREE(log);
	}

	asprintf(&log, "New connection: local:%d [socket:%d] --> %s:%d",
		ntohs(test->domain == AF_INET?
					((struct sockaddr_in *)&local_addr)->sin_port:
					((struct sockaddr_in6 *)&local_addr)->sin6_port),
		sockfd,	ip_address_str, test->server_port);
	PRINT_DBG_FREE(log);
	free(ip_address_str);
	freeaddrinfo(serv_info);

	if (test->is_sync_thread){
		if ((buffer = (char *)malloc(1 * sizeof(char))) == (char *)NULL){
			PRINT_ERR("cannot allocate memory for send buffer");
			close(sockfd);
			return 0;
		}
		memset(buffer, 'X', 1 * sizeof(char));
		n = n_write(sockfd, buffer, 1);
		if (n < 0) {
			PRINT_ERR("cannot write data to the socket for sender/receiver sync");
			free(buffer);
			close(sockfd);
			return 0;
		}
		bzero(buffer, 1);
		n = n_read(sockfd, buffer, 1);
		if (n < 0) {
			PRINT_ERR("cannot read data from the socket for sender/receiver sync");
			free(buffer);
			close(sockfd);
			return 0;
		}
		 // for debug
		/*
		for (i = 0; i < n; i++ ){
			putc( isprint(buffer[i]) ? buffer[i] : '.' , stdout );
			fflush(stdout);
		}
		*/
		if (n > 0) {
			if (buffer[0] == TEST_RUNNING) {
				PRINT_ERR("receiver is busy with an existing test running");
				free(buffer);
				close(sockfd);
				return 0;
			}
		}
		nbytes = 0; //don't count this bytes as it is synch thread

		/* now, now turn on the green light so that other sync thread can send data for test */
		turn_on_light();
		PRINT_INFO("Network activity progressing...");
	}
	else{
		/* wait for sync thread to finish */
		wait_light_on();

		if ((buffer = (char *)malloc(test->send_buf_size * sizeof(char))) == (char *)NULL){
			PRINT_ERR("cannot allocate memory for send buffer");
			close(sockfd);
			return 0;
		}
		//fill_buffer(buffer, test->send_buf_size);
		memset(buffer, 'A', test->send_buf_size * sizeof(char));

		while (is_light_turned_on()){
			n = n_write(sockfd, buffer, strlen(buffer));
			if (n < 0) {
				PRINT_ERR("cannot write data to a socket");
				free(buffer);
				close(sockfd);
				return 0;
			}
			nbytes += n;
		}
	}

	free(buffer);
	close(sockfd);

	return (void *)nbytes;
}

/************************************************************/
//		ntttcp receiver
/************************************************************/

int ntttcp_server_listen(struct ntttcp_stream_server *test)
{
	char *log;
	int opt = 1;
	int sockfd = 0; //socket file descriptor
	char *ip_address_str; //used to get local ip address
	int ip_address_max_size;  //used to get local ip address
	char *port_str; //to get remote peer's port number for getaddrinfo()
	struct addrinfo hints, *serv_info, *p; //to get remote peer's sockaddr for bind()

	int i = 0; //just for debug purpose

	/* bind to local address */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = test->domain;
	hints.ai_socktype = test->protocol;
	asprintf(&port_str, "%d", test->server_port);
	if (getaddrinfo(test->bind_address, port_str, &hints, &serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return -1;
	}
	free(port_str);

	ip_address_max_size = (test->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL){
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return -1;
	}

	/* get the first entry to bind and listen */
	for (p = serv_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0){
			PRINT_ERR("cannot create socket ednpoint");
			freeaddrinfo(serv_info);
			free(ip_address_str);
			return -1;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
			asprintf(&log, "cannot set socket options: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(ip_address_str);
			close(sockfd);
			return -1;
		}
		if ( set_socket_non_blocking(sockfd) == -1){
			asprintf(&log, "cannot set socket as non-blocking: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(ip_address_str);
			close(sockfd);
			return -1;
		}
		if (( i = bind(sockfd, p->ai_addr, p->ai_addrlen)) < 0){
			asprintf(&log, "failed to bind the socket to local address: %s on socket: %d. errcode = %d",
			ip_address_str = retrive_ip_address_str((struct sockaddr_storage *)p->ai_addr, ip_address_str, ip_address_max_size), sockfd, i );

			if (i == -1)
				asprintf(&log, "%s. errcode = %d", log, errno);
			PRINT_DBG_FREE(log);
			continue;
		}
		else{
			break; //connected
		}
	}
	freeaddrinfo(serv_info);
	free(ip_address_str);
	if (p == NULL){
		asprintf(&log, "cannot bind the socket on address: %s", test->bind_address);
		PRINT_ERR_FREE(log);
		close(sockfd);
		return -1;
	}

	test->listener = sockfd;
	if (listen(test->listener, MAX_CONNECTIONS_PER_THREAD) < 0){
		asprintf(&log, "failed to listen on address: %s: %d", test->bind_address, test->server_port);
		PRINT_ERR_FREE(log);
		close(test->listener);
		return -1;
	}

	FD_ZERO(&test->read_set);
	FD_ZERO(&test->write_set);
	FD_SET(test->listener, &test->read_set);
	if (test->listener > test->max_fd)
		test->max_fd = test->listener;

	asprintf(&log, "ntttcp server is listening on %s:%d", test->bind_address, test->server_port);
	PRINT_DBG_FREE(log);

	return test->listener;
}

int ntttcp_server_epoll(struct ntttcp_stream_server *test)
{
	int err_code = NO_ERROR;
	char *log = NULL;

	int efd = 0, n_fds = 0, newfd = 0, current_fd = 0;
	char *buffer; //receive buffer
	long nbytes;   //bytes read
	int bytes_to_be_read = 0;  //read bytes from socket
	struct epoll_event event, *events;

	struct sockaddr_storage peer_addr, local_addr; //for remote peer, and local address
	socklen_t peer_addr_size, local_addr_size;
	char *ip_address_str;
	int ip_address_max_size;
	int i = 0;
	char receiver_state = TEST_UNKNOWN;

	if ( (buffer = (char *)malloc(test->recv_buf_size)) == (char *)NULL){
		PRINT_ERR("cannot allocate memory for receive buffer");
		return ERROR_MEMORY_ALLOC;
	}
	ip_address_max_size = (test->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL){
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

	event.data.fd = test->listener;
	event.events = EPOLLIN;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, test->listener, &event) != 0){
		PRINT_ERR("epoll_ctl failed");
		free(buffer);
		free(ip_address_str);
		close(efd);
		return ERROR_EPOLL;
	}

	/* Buffer where events are returned */
	events = calloc (MAX_EPOLL_EVENTS, sizeof event);

	while (1){
		n_fds = epoll_wait (efd, events, MAX_EPOLL_EVENTS, -1);
		for (i = 0; i < n_fds; i++){
			current_fd = events[i].data.fd;

			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
				/* An error has occured on this fd, or the socket is not ready for reading */
				PRINT_ERR("error happened on the associated connection");
				close (current_fd);
				continue;
			}

			/* then, we got one fd to hanle */
			/* a NEW connection coming */
			if (current_fd == test->listener) {
				/* We have a notification on the listening socket, which means one or more incoming connections. */
				while (1) {
					peer_addr_size = sizeof (peer_addr);
					newfd = accept (test->listener, (struct sockaddr *) &peer_addr, &peer_addr_size);
					if (newfd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							/* We have processed all incoming connections. */
							break;
						}
						else {
							PRINT_ERR("error to accept new connections");
							break;
						}
					}
					if (set_socket_non_blocking(newfd) == -1){
						asprintf(&log, "cannot set the new socket as non-blocking: %d", newfd);
						PRINT_DBG_FREE(log);
					}

					local_addr_size = sizeof(local_addr);
					if (getsockname(newfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0){
						asprintf(&log, "failed to get local address information for the new socket: %d", newfd);
						PRINT_DBG_FREE(log);
					}
					else {
						asprintf(&log, "New connection: %s:%d --> local:%d [socket %d]",
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

					event.data.fd = newfd;
					event.events = EPOLLIN;
					if (epoll_ctl (efd, EPOLL_CTL_ADD, newfd, &event) != 0)
						PRINT_ERR("epoll_ctl failed");

					//if there is no synch thread, if any new connection coming, indicates test started
					if ( test->no_synch )
						turn_on_light();
					//else, leave the sync thread to fire the trigger
				}
			}
			/* handle data from an EXISTING client */
			else {
				bzero(buffer, test->recv_buf_size);
				bytes_to_be_read = test->is_sync_thread ? 1 : test->recv_buf_size;

				/* got error or connection closed by client */
				if ((nbytes = n_read(current_fd, buffer, bytes_to_be_read)) <= 0) {
					if (nbytes == 0){
						asprintf(&log, "socket closed: %d", i);
						PRINT_DBG_FREE(log);
					}
					else {
						asprintf(&log, "error: cannot read data from socket: %d", i);
						PRINT_INFO_FREE(log);
						err_code = ERROR_NETWORK_READ;
						/* need to continue test and check other socket, so don't end the test */
					}
					close (current_fd);
				}
				/* report how many bytes received */
				else {
					__sync_fetch_and_add(&(test->total_bytes_transferred), nbytes);

					if (test->is_sync_thread){
						// for debug, write the 'X' received from sender
						/*
						for ( j = 0; j < nbytes; j++ ){
							putc( isprint(buffer[j]) ? buffer[j] : '.' , stdout );
							fflush(stdout);
						}
						*/
						/* send ack to client on the sync thread */
						if (is_light_turned_on())
							receiver_state = TEST_RUNNING;
						else
							receiver_state = TEST_NOT_STARTED;

						memset(buffer, (char)receiver_state, 1 * sizeof(char));
						nbytes = n_write(current_fd, buffer, 1);
						if (nbytes < 0){
							PRINT_ERR("cannot write ack data to the socket for sender/receiver sync");
							err_code = ERROR_NETWORK_WRITE;
						}
						else {
							if (!is_light_turned_on()){
								turn_on_light();
								PRINT_INFO("Network activity progressing...");
							}
						}
					}
				}
			}
		}
	}

	free(buffer);
	free(ip_address_str);
	free(events);
	close(efd);
	close(test->listener);
	return err_code;
}

int ntttcp_server_select(struct ntttcp_stream_server *test)
{
	int err_code = NO_ERROR;
	char *log = NULL;

	int n_fds = 0, newfd, current_fd = 0;
	char *buffer; //receive buffer
	long nbytes; //bytes read
	int bytes_to_be_read = 0;  //read bytes from socket
	fd_set read_set, write_set;

	struct sockaddr_storage peer_addr, local_addr; //for remote peer, and local address
	socklen_t peer_addr_size, local_addr_size;
	char *ip_address_str;
	int ip_address_max_size;
	char receiver_state = TEST_UNKNOWN;

	if ( (buffer = (char *)malloc(test->recv_buf_size)) == (char *)NULL){
		PRINT_ERR("cannot allocate memory for receive buffer");
		return ERROR_MEMORY_ALLOC;
	}
	ip_address_max_size = (test->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (ip_address_str = (char *)malloc(ip_address_max_size)) == (char *)NULL){
		PRINT_ERR("cannot allocate memory for ip address of peer");
		free(buffer);
		return ERROR_MEMORY_ALLOC;
	}

	/* accept new client, receive data from client */
	while (1) {
		memcpy(&read_set, &test->read_set, sizeof(fd_set));
		memcpy(&write_set, &test->write_set, sizeof(fd_set));

		/* we are notified by select() */
		n_fds = select(test->max_fd + 1, &read_set, NULL, NULL, NULL);
		if (n_fds < 0 && errno != EINTR){
			PRINT_ERR("error happened when select()");
			err_code = ERROR_SELECT;
			continue;
		}

		/*run through the existing connections looking for data to be read*/
		for (current_fd = 0; current_fd <= test->max_fd; current_fd++){
			if ( !FD_ISSET(current_fd, &read_set) )
				continue;

			/* then, we got one fd to hanle */
			/* a NEW connection coming */
			if (current_fd == test->listener) {
 				/* handle new connections */
				peer_addr_size = sizeof(peer_addr);
				if ((newfd = accept(test->listener, (struct sockaddr *) &peer_addr, &peer_addr_size)) < 0 ){
					err_code = ERROR_ACCEPT;
					break;
				}

				/* then we got a new connection */
				if (set_socket_non_blocking(newfd) == -1){
					asprintf(&log, "cannot set the new socket as non-blocking: %d", newfd);
					PRINT_DBG_FREE(log);
				}
				FD_SET(newfd, &test->read_set); /* add the new one to read_set */
				if (newfd > test->max_fd){
					/* update the maximum */
					test->max_fd = newfd;
				}

				/* print out new connection info */
				local_addr_size = sizeof(local_addr);
				if (getsockname(newfd, (struct sockaddr *) &local_addr, &local_addr_size) != 0){
					asprintf(&log, "failed to get local address information for the new socket: %d", newfd);
					PRINT_DBG_FREE(log);
				}
				else{
					asprintf(&log, "New connection: %s:%d --> local:%d [socket %d]",
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

				//if there is no synch thread, if any new connection coming, indicates test started
				if ( test->no_synch )
					turn_on_light();
				//else, leave the sync thread to fire the trigger
			}
			/* handle data from an EXISTING client */
			else{
				bzero(buffer, test->recv_buf_size);
				bytes_to_be_read = test->is_sync_thread ? 1 : test->recv_buf_size;

				/* got error or connection closed by client */
				if ((nbytes = n_read(current_fd, buffer, bytes_to_be_read)) <= 0) {
					if (nbytes == 0){
						asprintf(&log, "socket closed: %d", current_fd);
						PRINT_DBG_FREE(log);
					}
					else{
						asprintf(&log, "error: cannot read data from socket: %d", current_fd);
						PRINT_INFO_FREE(log);
						err_code = ERROR_NETWORK_READ;
						/* need to continue test and check other socket, so don't end the test */
					}
					close(current_fd);
					FD_CLR(current_fd, &test->read_set); /* remove from master set when finished */
				}
				/* report how many bytes received */
				else{
					__sync_fetch_and_add(&(test->total_bytes_transferred), nbytes);

					if (test->is_sync_thread){
						// for debug, write the 'X' received from sender
						/*
						for ( j = 0; j < nbytes; j++ ){
							putc( isprint(buffer[j]) ? buffer[j] : '.' , stdout );
							fflush(stdout);
						}
						*/
						/* send ack to client on the sync thread */
						if (is_light_turned_on())
							receiver_state = TEST_RUNNING;
						else
							receiver_state = TEST_NOT_STARTED;

						memset(buffer, (char)receiver_state, 1 * sizeof(char));
						nbytes = n_write(current_fd, buffer, 1);
						if (nbytes < 0){
							PRINT_ERR("cannot write ack data to the socket for sender/receiver sync");
							err_code = ERROR_NETWORK_WRITE;
						}
						else{
							if (!is_light_turned_on()){
								turn_on_light();
								PRINT_INFO("Network activity progressing...");
							}
						}
					}
				}
			}
		}
	}

	free(buffer);
	free(ip_address_str);
	close(test->listener);
	return err_code;
}

void *run_ntttcp_receiver_stream( void *ptr )
{
	char *log = NULL;
	struct ntttcp_stream_server *test;

	test = (struct ntttcp_stream_server *) ptr;
	test->listener = ntttcp_server_listen(test);
	if (test->listener < 0){
		asprintf(&log, "listen error at port: %d", test->server_port);
		PRINT_ERR_FREE(log);
	}
	else{
		if (test->use_epoll == true){
			if ( ntttcp_server_epoll(test) != NO_ERROR ) {
				asprintf(&log, "epoll error at port: %d", test->server_port);
				PRINT_ERR_FREE(log);
			}
		}
		else {
			if ( ntttcp_server_select(test) != NO_ERROR ){
				asprintf(&log, "select error at port: %d", test->server_port);
				PRINT_ERR_FREE(log);
			}
		}
	}

	return NULL;
}

/************************************************************/
//		ntttcp high level functions
/************************************************************/
int run_ntttcp_sender(struct ntttcp_test_endpoint *tep)
{
	int err_code = NO_ERROR;
	struct ntttcp_test *test;
	char *log = NULL;
	int threads_created = 0, stream_created = 0;
	struct ntttcp_stream_client *this_client_stream;
	int rc, t, i;
	void *bytes;
	long nbytes = 0, total_bytes = 0;
	struct timeval now;
	double actual_test_time = 0;
	struct cpu_usage *init_cpu_usage, *final_cpu_usage;
	uint64_t init_cycle_count = 0, final_cycle_count = 0, cycle_diff = 0;

	test = tep->test;

	/* calculate the resource usage */
	init_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!init_cpu_usage){
		PRINT_ERR("receiver: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}
	final_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!final_cpu_usage){
		free (init_cpu_usage);
		PRINT_ERR("receiver: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}
	get_cpu_usage( init_cpu_usage );
	init_cycle_count = get_cc_rdtsc();

	/* create threads */
	for (t = 0; t < tep->test->parallel + 1; t++) {  //the last one is an extra synch thread
		this_client_stream = tep->client_streams[t];
		if (this_client_stream == NULL){
			PRINT_ERR("sender: error when creating new client stream");
			err_code = ERROR_MEMORY_ALLOC;
			continue;
		}
		this_client_stream->server_port = test->server_base_port + t;
		stream_created++;

		/* for the last iteration: */
		if (t == test->parallel) {
			if (test->no_synch == false){
				/* synch thread */
				this_client_stream->server_port = test->server_base_port - 1;
				this_client_stream->is_sync_thread = 1;
				rc = pthread_create(&tep->data_threads[threads_created],
							NULL,
							run_ntttcp_sender_stream,
							(void*)this_client_stream);
				if (rc){
					PRINT_ERR("pthread_create() create thread failed");
					err_code = ERROR_PTHREAD_CREATE;
					continue;
				}
				else{
					threads_created++;
				}
			}
			else{   /* no synch thread, so turn on light directly */
				turn_on_light();
			}
			continue; //anyway, finish the loop as this is the last thread
		}

		/* else, create normal work threads */
		/* in client side, multiple connections will (one thread for one connection) */
		/* connect to same port on server */
		for (i = 0; i < test->conn_per_thread; i++ ){
			rc = pthread_create(&tep->data_threads[threads_created],
						NULL,
						run_ntttcp_sender_stream,
						(void*)this_client_stream);
			if (rc){
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

	/* run the timer. it will trigger turn_off_light() after timer timeout */
	run_test_timer(test->duration);
	/* wait synch thread to complete */
	wait_light_on();
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
	for (i = 0; i < threads_created - 1; i++) {
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
	struct ntttcp_test *test;
	char *log = NULL;
	int total_threads = 0, threads_created = 0, stream_created = 0;
	struct ntttcp_stream_server *this_server_stream;
	int rc, t;
	long nbytes = 0, total_bytes = 0;
	struct timeval now;
	double actual_test_time = 0;
	struct cpu_usage *init_cpu_usage, *final_cpu_usage;
	uint64_t init_cycle_count = 0, final_cycle_count = 0, cycle_diff = 0;

	test = tep->test;

	/* calculate the resource usage */
	init_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!init_cpu_usage){
		PRINT_ERR("receiver: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}
	final_cpu_usage = (struct cpu_usage *) malloc(sizeof(struct cpu_usage));
	if (!final_cpu_usage){
		free (init_cpu_usage);
		PRINT_ERR("receiver: error when creating cpu_usage struct");
		return ERROR_MEMORY_ALLOC;
	}
	get_cpu_usage( init_cpu_usage );
	init_cycle_count = get_cc_rdtsc();

	/* create threads */
	total_threads = test->parallel + 1; //including one synch thread
	for (t = 0; t < total_threads; t++) {
		this_server_stream = tep->server_streams[t];
		if (this_server_stream == NULL){
			PRINT_ERR("receiver: error when creating new server stream");
			continue;
		}
		this_server_stream->server_port = test->server_base_port + t;
		stream_created++;

		if (t == total_threads - 1){
			if (test->no_synch == false){
				/* sync thread */
				this_server_stream->server_port = test->server_base_port - 1;
				this_server_stream->is_sync_thread = 1;
			}
			else{
				continue;
			}
		}

		rc = pthread_create(&tep->data_threads[t],
					NULL,
					run_ntttcp_receiver_stream,
					(void*)this_server_stream);
		if (rc){
			PRINT_ERR("pthread_create() create thread failed");
			err_code = ERROR_PTHREAD_CREATE;
			continue;
		}
		threads_created++;
	}

	asprintf(&log, "%d threads created", threads_created);
	PRINT_DBG_FREE(log);

	while ( 1 ) {
		if ( is_light_turned_on() ){ //means: at receiver side, testing is in progress, then just wait for its done
			/* run the timer. it will trigger turn_off_light() after timer timeout */
			run_test_timer(test->duration);

			/* wait synch thread to complete */
			//wait_light_on();  // test already started, don't need to wait
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

			/* reset server side perf counters */
			for (t=0; t < stream_created; t++)
				__atomic_store_n( &(tep->server_streams[t]->total_bytes_transferred), 0, __ATOMIC_SEQ_CST );
		}
		else { //means: at server side, test is not started, then just wait for its start
		}
	}

	/* as receiver threads will keep listening on ports, so they will not exit */
	for (t=0; t < threads_created; t++) {
		pthread_join(tep->data_threads[t], NULL);
	}

	free (init_cpu_usage);
	free (final_cpu_usage);

	return err_code;
}

/* for debug with single thread */
int run_ntttcp_debug(struct ntttcp_test *test)
{
	int err_code = NO_ERROR;
	struct ntttcp_stream_client *client_stream;
	struct ntttcp_stream_server *server_stream;
	if (test->client_role == true){
		client_stream = new_ntttcp_client_stream(test);
		turn_on_light();
		client_stream->server_port = 5001;
		run_ntttcp_sender_stream(client_stream);
	}
	else{
		server_stream = new_ntttcp_server_stream(test);
		server_stream->server_port = 5001;
		run_ntttcp_receiver_stream(server_stream);
	}
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
	if (!test){
		PRINT_ERR("main: error when creating new test");
		exit (-1);
	}

	default_ntttcp_test(test);
	err_code = parse_arguments(test, argc, argv);
	if (err_code != NO_ERROR){
		PRINT_ERR("main: error when parsing args");
		print_flags(test);
		free(test);
		exit (-1);
	}

	err_code = verify_args(test);
	if (err_code != NO_ERROR){
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

	if (test->daemon){
		PRINT_INFO("main: run this tool in the background");
		if ( daemon(0, 0) != 0 )
			PRINT_ERR("main: cannot run this tool in the background");
	}

//	run_ntttcp_debug(test);

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
