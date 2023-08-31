#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../include/string_array.h"
#include "../include/string_utils.h"
#include "../include/socket.h"
#include "../include/command_handler.h"
#include "../include/fork_handler.h"

int handle_client(int socket)
{
    while (1)
    {
        char *command = receive_command(socket);
        if (command == NULL) {
            break;
        }

        if (!validate_command(command)) {
            free(command);
            continue;
        }

        handle_command(socket, command);

        free(command);
    }
    close(socket);
    return 0;
}

void fork_strategy(int server_socket)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    int client_socket, state;
    pid_t pid;
    while (1)
    {
        client_addr_size = sizeof client_addr;
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socket == -1)
        {
            perror("accept");
            continue;
        }

        pid = fork();
        if (pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            // Child
            pid = fork();
            if (pid < 0)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if (pid == 0)
            {
                // Grandchild
                handle_client(client_socket);
            }
            close(client_socket);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // Parent
            waitpid(pid, &state, 0);
            close(client_socket);
        }
    }
}

