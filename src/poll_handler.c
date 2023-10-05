#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/command_handler.h"
#include "../include/pfd_array.h"
#include "../include/poll_handler.h"
#include "../include/socket.h"

#define COMMAND_LEN 128

void poll_strategy(int server_socket)
{
    int status = set_socket_nonblocking(server_socket);
    if (status == -1) {
        return;
    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    struct pfd_array* arr = pfd_array_new();
    pfd_array_insert(arr, server_socket);

    while (true) {
        if (poll(arr->data, arr->count, -1) == -1 && errno != EINTR) {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < arr->count; i++) {
            if (arr->data[i].revents & POLLERR) {
                fprintf(stderr, "Error: (FD %d) revents = %d\n", arr->data[i].fd, arr->data[i].revents);
                close(arr->data[i].fd);
                continue;
            }
            if (arr->data[i].revents & POLLHUP) {
                fprintf(stderr, "(FD %d) closed connection.\n", arr->data[i].fd);
                close(arr->data[i].fd);
                continue;
            }
            if (arr->data[i].revents & POLLIN) {
                int fd = arr->data[i].fd;
                if (fd == server_socket) {
                    client_addr_size = sizeof client_addr;
                    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
                    if (client_socket == -1) {
                        // Since we are using non-blocking sockets, if accept() returns EWOULDBLOCK or EAGAIN,
                        // then there are no connections waiting to be accepted. In this case, we can simply move on.
                        if (errno != EWOULDBLOCK || errno != EAGAIN) {
                            perror("accept");
                            pfd_array_free(arr);
                            return;
                        }
                    }
                    pfd_array_insert(arr, client_socket);
                    fprintf(stdout, "New connection accepted.\n");
                } else {
                    char* command = receive_command(fd);
                    if (command == NULL) {
                        pfd_array_remove(arr, i);
                        continue;
                    }
                    handle_command(fd, command);
                    free(command);
                }
            }
        }
    }

    pfd_array_free(arr);
}
