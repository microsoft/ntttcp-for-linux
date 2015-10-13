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

	while (left > 0){
		rtn = read(fd, buffer, left);
		if (rtn < 0){
			if (errno == EINTR || errno == EAGAIN)
				break;
			else
				return ERROR_NETWORK_READ;
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

	while (left > 0){
		rtn = write(fd, buffer, left);
		if (rtn < 0){
			if (errno == EINTR || errno == EAGAIN)
				return total - left;
			else
				return ERROR_NETWORK_WRITE;
		}
		else if (rtn == 0)
			return ERROR_NETWORK_WRITE;

		left -= rtn;
		buffer += rtn;
	}
	return total;
}

int set_socket_non_blocking(int fd)
{
	int flags, rtn;
	flags = fcntl (fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	flags |= O_NONBLOCK;
	rtn = fcntl (fd, F_SETFL, flags);
	if (rtn == -1)
		return -1;

	return 0;
}
