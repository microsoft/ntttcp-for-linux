#ifndef EPOLL_COMPAT_H
#define EPOLL_COMPAT_H

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define EPOLLIN EVFILT_READ
#define EPOLLOUT EVFILT_WRITE
#define EPOLLET EV_CLEAR
#define EPOLLONESHOT EV_ONESHOT
#define EPOLLPRI EVFILT_EXCEPT
#define EPOLLHUP EV_EOF     // Equivalent to EV_EOF for kqueue
#define EPOLLERR EV_ERROR   // Equivalent to EV_ERROR for kqueue
#define EPOLL_CTL_ADD EV_ADD
#define EPOLL_CTL_DEL EV_DELETE
#define EPOLL_CTL_MOD EV_ENABLE

#define EPOLL_CLOEXEC 02000000

struct epoll_event {
    uint32_t events;
    uint64_t data;
};

int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif /* EPOLL_COMPAT_H */

