// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "udpstream.h"

/************************************************************/
//		UDP sender functions
/************************************************************/
void *run_ntttcp_sender_udp_stream( void *ptr )
{
	struct ntttcp_stream_client *sc = (struct ntttcp_stream_client *) ptr;
	if (sc->domain == AF_INET) {
		return run_ntttcp_sender_udp4_stream(sc);
	}
	else if (sc->domain == AF_INET6) {
		PRINT_ERR("UDP in IPv6 is not implemented yet");
		return 0;
	}
	else {
		PRINT_ERR("unsupported address family");
		return 0;
	}
}

void *run_ntttcp_sender_udp4_stream( struct ntttcp_stream_client * sc )
{
	char *log        = NULL;
	int sockfd       = 0; //socket id
	char *buffer;         //send buffer

	int n            = 0; //write n bytes to socket
	uint64_t nbytes  = 0; //total bytes sent
	int i            = 0; //hold function return value
	bool verbose_log = sc->verbose;

	struct sockaddr_in local_addr, serv_addr;
	int sa_size = sizeof(struct sockaddr_in);
	struct hostent *hp = gethostbyname(sc->bind_address);
	memset((char*)&serv_addr, 0, sa_size);
	serv_addr.sin_family = sc->domain; //AF_INET
	serv_addr.sin_port = htons(sc->server_port);
	memcpy((void *)&serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

	if ((sockfd = socket(sc->domain, sc->domain, 0)) < 0){
		PRINT_ERR("cannot create socket ednpoint");
		return 0;
	}

	/*
	   2. bind this socket fd to a local random/ephemeral TCP port,
	      so that the sender side will have randomized TCP ports.
	*/
	(*(struct sockaddr_in*)&local_addr).sin_family = sc->domain; //AF_INET
	(*(struct sockaddr_in*)&local_addr).sin_port = 0;

	if (( i = bind(sockfd, (struct sockaddr *)&local_addr, sa_size)) < 0 ){
		ASPRINTF(&log, "failed to bind socket: %d to a local ephemeral port. errno = %d", sockfd, errno);
		PRINT_ERR_FREE(log);
		return 0;
	}

	/* 3. send to receiver */
	/* wait for sync thread to finish */
	wait_light_on();

	ASPRINTF(&log, "Running UDP stream: local:%d [socket:%d] --> %s:%d",
		ntohs(((struct sockaddr_in *)&local_addr)->sin_port),
		sockfd,
		sc->bind_address,
		sc->server_port);
	PRINT_DBG_FREE(log);

	if ((buffer = (char *)malloc( sc->send_buf_size * sizeof(char))) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for send buffer");
		close(sockfd);
		return 0;
	}

	memset(buffer, 'B', sc->send_buf_size * sizeof(char));
	while (is_light_turned_on(sc->continuous_mode)){
		n = sendto(sockfd, buffer, sc->send_buf_size, 0, (struct sockaddr *)&serv_addr, sa_size);
		if (n < 0) {
			PRINT_ERR("cannot write data to a socket");
			printf("error: %d \n", errno);
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

//void *run_ntttcp_sender_udp6_stream( struct ntttcp_stream_client * sc )
//{
//	return 0;
//}

/************************************************************/
//		UDP receiver functions
/************************************************************/
void *run_ntttcp_receiver_udp_stream( void *ptr )
{
	struct ntttcp_stream_server *ss  = (struct ntttcp_stream_server *) ptr;
	if (ss->domain == AF_INET) {
		return run_ntttcp_receiver_udp4_stream(ss);
	}
	else if (ss->domain == AF_INET6) {
		PRINT_ERR("UDP in IPv6 is not implemented yet");
		return 0;
	}
	else {
		PRINT_ERR("unsupported address family");
		return 0;
	}
}

void *run_ntttcp_receiver_udp4_stream( struct ntttcp_stream_server * ss )
{
	char *log;
	bool verbose_log = false;

	int i            = 0;  //hold function return value
//	int opt          = 1;
	int sockfd       = 0;  //socket file descriptor
	char *buffer;          //receive buffer
	char *local_addr_str;  //used to get local ip address
	int ip_addr_max_size;  //used to get local ip address
	char *port_str;        //used to get port number string for getaddrinfo()
	struct addrinfo hints, *serv_info, *p; //to get local sockaddr for bind()
	struct sockaddr_in remote_addr;	          // remote address
	socklen_t addrlen = sizeof(remote_addr);  // length of addresses
	ssize_t nbytes    = 0; //bytes received

	verbose_log = ss->verbose;

	/* get receiver/itself address */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = ss->domain;
	hints.ai_socktype = ss->protocol;

	ASPRINTF(&port_str, "%d", ss->server_port);
	if (getaddrinfo(ss->bind_address, port_str, &hints, &serv_info) != 0) {
		PRINT_ERR("cannot get address info for receiver");
		return 0;
	}
	free(port_str);

	ip_addr_max_size = (ss->domain == AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ( (local_addr_str = (char *)malloc(ip_addr_max_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		freeaddrinfo(serv_info);
		return 0;
	}

	/* get the first entry to bind and listen */
	for (p = serv_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, UDP, p->ai_protocol)) < 0) {
			PRINT_ERR("cannot create socket ednpoint");
			freeaddrinfo(serv_info);
			free(local_addr_str);
			return 0;
		}

		/*
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
			ASPRINTF(&log, "cannot set socket options: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(local_addr_str);
			close(sockfd);
			return 0;
		}
		if ( set_socket_non_blocking(sockfd) == -1) {
			ASPRINTF(&log, "cannot set socket as non-blocking: %d", sockfd);
			PRINT_ERR_FREE(log);
			freeaddrinfo(serv_info);
			free(local_addr_str);
			close(sockfd);
			return 0;
		}
		*/

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
		return 0;
	}

	if ( (buffer = (char *)malloc(ss->recv_buf_size)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for receive buffer");
		close(sockfd);
		return 0;
	}

	/* wait for sync thread to finish */
	wait_light_on();

	for (;;) {
		bzero(buffer, ss->recv_buf_size);
		nbytes = recvfrom(sockfd, buffer, ss->recv_buf_size, 0, (struct sockaddr *)&remote_addr, &addrlen);
		if (nbytes > 0) {
			__sync_fetch_and_add(&(ss->total_bytes_transferred), nbytes);
		}
		else {
			ASPRINTF(&log, "error: cannot read data from socket: %d", sockfd);
			PRINT_INFO_FREE(log);
		}
	}
	/* never exits */
	return (void *)nbytes;
}

//void *run_ntttcp_receiver_udp6_stream( struct ntttcp_stream_server * ss )
//{
//	return 0;
//}
