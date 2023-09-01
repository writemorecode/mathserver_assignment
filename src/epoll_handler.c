#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/command_handler.h"
#include "../include/epoll_handler.h"
#include "../include/socket.h"

#define COMMAND_LEN 128
#define MAX_EVENTS 64

void epoll_strategy(int listen_socket)
{
    int status = set_socket_nonblocking(listen_socket);
    if (status == -1) {
        return;
    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    struct epoll_event* events = calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (events == NULL) {
        return;
    }

    int epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        free(events);
        return;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.data.fd = listen_socket;
    ev.events = EPOLLIN | EPOLLET;

    status = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_socket, &ev);
    if (status == -1) {
        perror("epoll_ctl");
        free(events);
        close(epfd);
        return;
    }

    while (true) {
        int ready_count = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (ready_count == -1) {
            perror("epoll");
            break;
        }

        for (int i = 0; i < ready_count; i++) {
            if (events[i].events & EPOLLERR) {
                fprintf(stderr, "Error: (FD %d) Events: %d\n", events[i].data.fd, events[i].events);
                close(events[i].data.fd);
                continue;
            }
            if (events[i].events & EPOLLHUP) {
                fprintf(stderr, "(FD %d) closed connection.\n", events[i].data.fd);
                close(events[i].data.fd);
                continue;
            }
            if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd;
                if (fd == listen_socket) {
                    int new_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_size);
                    if (new_socket == -1) {
                        // Since we are using non-blocking sockets, if accept() returns EWOULDBLOCK or EAGAIN,
                        // then there are no connections waiting to be accepted. In this case, we can simply move on.
                        if (errno != EWOULDBLOCK || errno != EAGAIN) {
                            perror("accept");
                            free(events);
                            return;
                        }
                    }

                    ev.data.fd = new_socket;
                    ev.events = EPOLLIN | EPOLLET;
                    status = epoll_ctl(epfd, EPOLL_CTL_ADD, new_socket, &ev);
                    if (status == -1) {
                        perror("epoll_ctl: new_socket");
                        break;
                    }
                    fprintf(stdout, "New connection accepted.\n");
                } else {
                    char* command = receive_command(fd);
                    if (command == NULL) {
                        continue;
                    }
                    handle_command(fd, command);
                    free(command);
                }
            }
        }
    }
    free(events);
    close(epfd);
}
