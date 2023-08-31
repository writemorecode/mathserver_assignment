#include <poll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/poll_handler.h"
#include "../include/socket.h"
#include "../include/pfd_array.h"
#include "../include/command_handler.h"

#define COMMAND_LEN 128

void poll_strategy(int server_socket)
{
    int status = set_socket_nonblocking(server_socket);
    if (status == -1) {
        return;
    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    struct pfd_array *arr = pfd_array_new(2);
    pfd_array_insert(arr, server_socket);

    while (1)
    {
        int poll_count = poll(arr->data, arr->count, -1);
        if (poll_count == -1)
        {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < arr->count; i++)
        {
            if (arr->data[i].revents & POLLIN)
            {
                int fd = arr->data[i].fd;
                if (fd == server_socket)
                {
                    client_addr_size = sizeof client_addr;
                    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
                    if (client_socket == -1)
                    {
                        perror("accept");
                        continue;
                    }
                    printf("DEBUG: inserting fd %d into arr with size %ld capacity %ld\n", client_socket, arr->count, arr->capacity);
                    pfd_array_insert(arr, client_socket);

                    fprintf(stdout, "New connection recieved.\n");
                }
                else
                {
                    char *command = calloc(COMMAND_LEN + 1, sizeof(char));
                    int n_recv = recv(fd, command, COMMAND_LEN, 0);
                    if (n_recv <= 0) {
                        if (n_recv == -1) {
                            perror("recv");
                        }
                        pfd_array_remove(arr, i);
                        close(fd);
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
