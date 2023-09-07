#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "epoll.h"
#include <fcntl.h>
// A wrapper struct for storing an epoll event and its associated file descriptor
struct epollev {
    int fd;
    struct epoll_event event;
};

// A helper function for creating a struct epollev
struct epollev *epollev_new(int fd, struct epoll_event *event) {
    struct epollev *ev = malloc(sizeof(struct epollev));
    if (ev == NULL) {
        perror("malloc");
        return NULL;
    }
    ev->fd = fd;
    ev->event = *event;
    return ev;
}

// A helper function for freeing a struct epollev
void epollev_free(struct epollev *ev) {
    free(ev);
}



// A wrapper function for creating an epoll instance using kqueue
int epoll_create1(int flags) {
    // Create a kqueue instance
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        return -1;
    }

    // Set the close-on-exec flag if requested
    if (flags & EPOLL_CLOEXEC) {
        int fflags = fcntl(kq, F_GETFD);
        if (fflags == -1) {
            perror("fcntl");
            close(kq);
            return -1;
        }
        fflags |= FD_CLOEXEC;
        if (fcntl(kq, F_SETFD, fflags) == -1) {
            perror("fcntl");
            close(kq);
            return -1;
        }
    }

    // Return the kqueue file descriptor as the epoll file descriptor
    return kq;
}

// A wrapper function for modifying the file descriptors in the epoll instance using kevent
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    // Create a struct kevent for the change
    struct kevent kev;
    EV_SET(&kev, fd, event->events, op, 0, 0, epollev_new(fd, event));

    // Apply the change to the kqueue instance
    if (kevent(epfd, &kev, 1, NULL, 0, NULL) == -1) {
        perror("kevent");
        return -1;
    }

    // Return 0 on success
    return 0;
}

// A wrapper function for waiting for I/O events on the file descriptors in the epoll instance using kevent
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    // Create an array of struct kevent for the events
    struct kevent *kevs = malloc(sizeof(struct kevent) * maxevents);
    if (kevs == NULL) {
        perror("malloc");
        return -1;
    }

    // Create a struct timespec for the timeout
    struct timespec ts;
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
    }

    // Wait for events on the kqueue instance
    int nevents = kevent(epfd, NULL, 0, kevs, maxevents, timeout >= 0 ? &ts : NULL);
    if (nevents == -1) {
        perror("kevent");
        free(kevs);
        return -1;
    }

    // Convert the events from struct kevent to struct epoll_event
    for (int i = 0; i < nevents; i++) {
        struct epollev *ev = (struct epollev *)kevs[i].udata;
        events[i].data = ev->event.data;
        events[i].events = kevs[i].filter;
        epollev_free(ev);
    }

    // Free the array of struct kevent
    free(kevs);

    // Return the number of events
    return nevents;
}

