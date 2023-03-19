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
#include "../include/string_array.h"
#include "../include/net.h"

#define COMMAND_BUF_LEN 256
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

    char *program_name = get_program_name(command);

    // Send command to server
    ssize_t send_ret = send(socket, command, send_len, 0);
    if (send_ret == -1)
    {
        free(program_name);
        perror("send");
        return EXIT_FAILURE;
    }

    char *input_filename = NULL;
    // If sending a kmeans command, send input data to server
    if (strcmp(program_name, "kmeanspar") == 0)
    {
        struct string_array *array = string_array_from_string(command, " ");
        for (size_t i = 0; i < array->size; i++)
        {
            if (strcmp(array->data[i], "-f") == 0 && array->size > i + 1)
            {
                input_filename = array->data[i + 1];
                break;
            }
        }

        if (input_filename == NULL)
        {
            string_array_free(array);
            return EXIT_FAILURE;
        }

        FILE *fp = fopen(input_filename, "r");
        if (fp == NULL)
        {
            free(program_name);
            perror("fopen");
            return EXIT_FAILURE;
        }

        fseek(fp, 0, SEEK_END);
        size_t input_data_size = ftell(fp);
        rewind(fp);

        char *input_buffer = calloc(input_data_size, sizeof(char));
        if (input_buffer == NULL)
        {
            perror("calloc");
            return EXIT_FAILURE;
        }
        size_t input_read_total = 0;
        char *input_ret;
        while (input_read_total < input_data_size)
        {
            input_ret = fgets(input_buffer + input_read_total, input_data_size - input_read_total, fp);
            if (input_ret == NULL)
            {
                break;
            }
        }

        send(socket, &input_data_size, sizeof(input_data_size), 0);
        ssize_t send_total = 0;
        do
        {
            send_ret = send(socket, input_buffer + send_total, input_read_total - send_total, 0);
            if (send_ret == -1)
            {
                perror("send");
                break;
            }
            if (send_ret == 0)
            {
                break;
            }
            send_total += send_ret;
        } while (send_total < input_read_total);

        free(input_buffer);
        string_array_free(array);
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

    // Recieve solution data
    char *solution_buffer = read_full(socket, solution_length);
    size_t recv_total = solution_length;

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

    char *command = calloc(COMMAND_BUF_LEN, sizeof(char));
    char *prefix = "./";
    size_t prefix_length = strlen(prefix);
    memcpy(command, prefix, prefix_length);

    char *command_ptr;
    while (1)
    {
        fprintf(stdout, "Enter a command: ");
        command_ptr = fgets(command + prefix_length, COMMAND_BUF_LEN - prefix_length, stdin);
        if (command_ptr == NULL)
        {
            free(command);
            fprintf(stderr, "Error: fgets\n");
            break;
        }

        if (strlen(command) == 1 && command[0] == '\n')
        {
            continue;
        }

        int command_ret = handle_command(socket, command);

        if (command_ret == EXIT_QUIT)
        {
            break;
        }

        memset(command + prefix_length, 0, COMMAND_BUF_LEN - prefix_length);
    }
    close(socket);
    free(command);
}
