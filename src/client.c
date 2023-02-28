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

char *make_filename_string(const char *program)
{
    const char *fmt = "computed_results/%s_output_%s.txt";
    char *random_string = random_alphanumeric_string(8);
    const size_t total_len = snprintf(NULL, 0, fmt, program, random_string);
    char *str = calloc(total_len + 1, sizeof(char));
    snprintf(str, total_len + 1, fmt, program, random_string);
    free(random_string);
    return str;
}

char *get_program_name(char *command)
{
    if (command == NULL)
    {
        return NULL;
    }
    size_t program_name_length = 0;
    char *first_space = strchr(command, ' ');
    if (first_space == NULL)
    {
        program_name_length = strlen(command);
    }
    else
    {
        program_name_length = first_space - command;
    }

    char *buf = calloc(program_name_length + 1, sizeof(char));
    if (buf == NULL)
    {
        return NULL;
    }
    memcpy(buf, command, program_name_length);

    return buf;
}

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

int handle_command(int socket, char *command)
{
    size_t send_len = strlen(command);
    if (command[send_len - 1] == '\n')
    {
        command[send_len - 1] = 0;
    }

    if (strcmp(command, "quit") == 0)
    {
        fprintf(stdout, "Closing server connection.\n");
        send(socket, "quit", 4 + 1, 0);
        return EXIT_QUIT;
    }

    if (strcmp(command, "shutdown") == 0)
    {
        fprintf(stdout, "Shutting down server.\n");
        send(socket, "shutdown", 8 + 1, 0);
        return EXIT_QUIT;
    }

    char *program_name = get_program_name(command);

    // Send command to server
    ssize_t send_ret = send(socket, command, send_len, 0);
    if (send_ret == -1)
    {
        free(program_name);
        perror("send");
        return EXIT_FAILURE;
    }

    // Recieve length of solution data
    ssize_t solution_length = 0;

    ssize_t recv_ret = recv(socket, &solution_length, sizeof(size_t), 0);
    if (recv_ret == -1)
    {
        free(program_name);
        printf("Error: Could not recieve length of solution data.");
        perror("recv");
        return EXIT_FAILURE;
    }
    printf("DEBUG: solution length: %ld\n", solution_length);

    char *solution_buffer = calloc(solution_length, sizeof(char));
    if (solution_buffer == NULL)
    {
        free(program_name);
        perror("calloc");
        return EXIT_FAILURE;
    }

    ssize_t solution_buffer_length = solution_length;
    ssize_t recv_total = 0;

    // Recieve solution data
    while (recv_total < solution_length)
    {
        if (recv_total >= solution_buffer_length)
        {
            char *ret = realloc(solution_buffer, solution_buffer_length * 2);
            if (ret == NULL)
            {
                perror("realloc");
                return EXIT_FAILURE;
            }
            else
            {
                solution_buffer = ret;
            }
            solution_buffer_length *= 2;
        }
        recv_ret = recv(socket, solution_buffer + recv_total, BUF_LEN, 0);
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

    mkdir("computed_results", 0755);
    char *filename = make_filename_string(program_name);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("fopen");
    }

    size_t n = fwrite(solution_buffer, sizeof(char), recv_total, fp);
    fclose(fp);

    printf("Wrote %ld bytes to file '%s'.\n", n, filename);

    free(solution_buffer);
    free(program_name);
    free(filename);

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
        fprintf(stderr, "Failed to connect to server.\n");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Connected to server.\n");

    char *command = calloc(BUF_LEN, sizeof(char));
    char *command_ptr;
    while (1)
    {
        fprintf(stdout, "Enter a command: ");
        command_ptr = fgets(command, BUF_LEN, stdin);
        if (command_ptr == NULL)
        {
            free(command);
            fprintf(stderr, "Error: fgets\n");
            break;
        }

        if(strlen(command) == 1 && command[0] == '\n')
        {
            continue;
        }

        int command_ret = handle_command(socket, command);

        if (command_ret == EXIT_QUIT)
        {
            break;
        }

        memset(command, 0, BUF_LEN);
    }
    close(socket);
    free(command);
}
