#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/string_utils.h"

#define BUF_LEN 4096
#define EXIT_QUIT 1

int get_connect_socket(char *host, char *port)
{
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    int status = getaddrinfo(host, port, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, "Host not found: %s\n", host);
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    int connect_socket;
    for (p = servinfo; p != NULL; p = servinfo->ai_next)
    {
        connect_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (connect_socket == -1)
        {
            continue;
        }

        status = connect(connect_socket, p->ai_addr, p->ai_addrlen);
        if (status == -1)
        {
            close(connect_socket);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        // Could not connect to server
        return -1;
    }

    return connect_socket;
}

int handle_command(int socket, char *buffer)
{
    size_t send_len = strlen(buffer);
    if (buffer[send_len - 1] == '\n')
    {
        buffer[send_len - 1] = 0;
    }

    if (strcmp(buffer, "quit") == 0)
    {
        fprintf(stdout, "Closing server connection.\n");
        send(socket, "quit", 4 + 1, 0);
        return EXIT_QUIT;
    }

    if (strcmp(buffer, "shutdown") == 0)
    {
        fprintf(stdout, "Shutting down server.\n");
        send(socket, "shutdown", 8 + 1, 0);
        return EXIT_QUIT;
    }

    // Send command to server
    ssize_t send_ret = send(socket, buffer, send_len, 0);
    if (send_ret == -1)
    {
        perror("send");
        return EXIT_FAILURE;
    }

    // Recieve length of solution data
    ssize_t solution_length = 0;
    ssize_t buffer_length = BUF_LEN;
    ssize_t recv_total = 0;

    ssize_t recv_ret = recv(socket, &solution_length, sizeof(size_t), 0);
    if (recv_ret == -1)
    {
        printf("Error: Could not recieve length of solution data.");
        perror("recv");
        return EXIT_FAILURE;
    }
    printf("DEBUG: solution length: %ld\n", solution_length);

    // Recieve solution data
    while (recv_total < solution_length)
    {
        if (recv_total >= buffer_length)
        {
            char *ret = realloc(buffer, buffer_length * 2);
            if (ret == NULL)
            {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            else
            {
                buffer = ret;
            }
            memset(buffer + recv_total, 0, buffer_length);
            buffer_length *= 2;
        }
        recv_ret = recv(socket, buffer, BUF_LEN, 0);
        if (recv_ret == -1)
        {
            perror("read");
            break;
        }
        if (recv_ret == 0)
        {
            break;
        }
        recv_total += recv_ret;
        printf("DEBUG: recieved %ld bytes, total %ld\n", recv_ret, recv_total);
    }

    printf("Server response:\n");
    printf("%s\n", buffer);

    FILE *fp = fopen("computed_results/out.txt", "w");
    if (fp == NULL)
    {
        perror("fopen");
    }
    
    fwrite(buffer, sizeof(char), recv_total, fp);
    fclose(fp);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <address> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int socket = get_connect_socket(argv[1], argv[2]);
    if (socket == -1)
    {
        fprintf(stderr, "Failed to connect to server.");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Connected to server.\n");

    char *buffer_ptr;
    while (1)
    {
        char *buffer = calloc(BUF_LEN, sizeof(char));
        fprintf(stdout, "Enter a command: ");
        buffer_ptr = fgets(buffer, BUF_LEN, stdin);
        if (buffer_ptr == NULL)
        {
            fprintf(stderr, "Error: fgets\n");
            break;
        }

        int command_ret = handle_command(socket, buffer);
        free(buffer);

        if (command_ret == EXIT_QUIT)
        {
            break;
        }
    }
    close(socket);
}
