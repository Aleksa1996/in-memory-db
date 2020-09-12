#include <sys/epoll.h>

/**
 * Create event instance and add to watch list of epoll
 */
void epoll_ctl_add(int epoll, int socket)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = socket;

    epoll_ctl(epoll, EPOLL_CTL_ADD, socket, &event);
}