#include <sys/poll.h>

int main() {
  struct epoll_event event;
  int epoll_fd;

  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    perror("epoll_create");
    return 1;
  }

  event.events = EPOLLIN;
  event.data.fd = 0; // stdin

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event) == -1) {
    perror("epoll_ctl");
    return 1;
  }

  for (;;) {
    struct epoll_event events[1];
    int n;

    n = epoll_wait(epoll_fd, events, 1, -1);
    if (n == -1) {
      perror("epoll_wait");
      return 1;
    }

    if (events[0].events & EPOLLIN) {
      char buffer[1024];
      int nread;

      nread = read(0, buffer, sizeof(buffer));
      if (nread == -1) {
        perror("read");
        return 1;
      }

      write(1, buffer, nread);
    }
  }

  close(epoll_fd);

  return 0;
}

