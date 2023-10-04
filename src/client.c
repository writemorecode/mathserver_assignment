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

#include "../include/net.h"
#include "../include/string_array.h"
#include "../include/string_utils.h"

#define COMMAND_BUF_LEN 256
#define BUF_LEN 4096

char* make_filename_string(const char* program)
{
    const char* fmt = "computed_results/%s_output_%s.txt";
    char* random_string = random_alphanumeric_string(8);
    const size_t total_len = snprintf(NULL, 0, fmt, program, random_string);
    char* str = calloc(total_len + 1, sizeof(char));
    snprintf(str, total_len + 1, fmt, program, random_string);
    free(random_string);
    return str;
}

void write_server_response_to_file(char* filename, char* buffer, size_t buffer_length)
{
    const char* directory_name = "computed_results";
    int ret = mkdir(directory_name, 0755);
    if (ret != 0) {
        if (ret == EACCES) {
            fprintf(stderr, "Error: Denied permission to create directory '%s'.\n", directory_name);
            return;
        }
    }

    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        free(filename);
        return;
    }
    fwrite(buffer, sizeof(char), buffer_length, fp);
    fclose(fp);
}

int get_connect_socket(char* host, char* port)
{
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    int status = getaddrinfo(host, port, &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "Host not found: %s\n", host);
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    int connect_socket;
    for (p = servinfo; p != NULL; p = servinfo->ai_next) {
        connect_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (connect_socket == -1) {
            continue;
        }

        status = connect(connect_socket, p->ai_addr, p->ai_addrlen);
        if (status == -1) {
            close(connect_socket);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);

    if (p == NULL) {
        // Could not connect to server
        return -1;
    }

    return connect_socket;
}

int handle_command(int socket, char* command)
{
    strip_newline_from_end(command);
    size_t send_len = strlen(command);

    char* program_name = get_program_name(command);

    // Send command to server
    ssize_t send_ret = send(socket, command, send_len, 0);
    if (send_ret == -1) {
        free(program_name);
        perror("send");
        return EXIT_FAILURE;
    }

    char* input_filename = NULL;
    // If sending a kmeans command, send input data to server
    if (strcmp(program_name, "kmeanspar") == 0) {
        struct string_array* array = split_string(command, ' ');
        for (size_t i = 0; i < array->size; i++) {
            if (strcmp(array->data[i], "-f") == 0 && array->size > i + 1) {
                input_filename = array->data[i + 1];
                break;
            }
        }

        if (input_filename == NULL) {
            string_array_free(array);
            return EXIT_FAILURE;
        }

        int fd = open(input_filename, O_RDONLY | O_CREAT, 0644);
        if (fd == -1) {
            free(program_name);
            perror("open");
            return EXIT_FAILURE;
        }

        struct stat st;
        stat(input_filename, &st);
        size_t input_data_size = st.st_size;

        char* input_buffer = read_full(fd, input_data_size);
        if (input_buffer == NULL) {
            free(program_name);
            free(input_buffer);
            fprintf(stderr, "Error: Could not read input file '%s'.\n", input_filename);
            return EXIT_FAILURE;
        }

        // Send length of input data to server
        send(socket, &input_data_size, sizeof(input_data_size), 0);

        int ret = write_full(socket, input_buffer, input_data_size);
        if (ret == -1) {
            free(program_name);
            free(input_buffer);
            fprintf(stderr, "Error: Could not send input data to server.\n");
            return EXIT_FAILURE;
        }

        free(input_buffer);
        string_array_free(array);
    }

    // Recieve length of solution data
    ssize_t solution_length = 0;
    ssize_t recv_ret = recv(socket, &solution_length, sizeof(size_t), 0);
    if (recv_ret == -1) {
        free(program_name);
        printf("Error: Could not recieve length of solution data.");
        perror("recv");
        return EXIT_FAILURE;
    }

    // Recieve solution data
    char* solution_buffer = read_full(socket, solution_length);
    if (solution_buffer == NULL) {
        free(program_name);
        fprintf(stderr, "Error: Could not recieve solution data from server.\n");
        return EXIT_FAILURE;
    }

    char* filename = make_filename_string(program_name);
    write_server_response_to_file(filename, solution_buffer, solution_length);

    free(solution_buffer);
    free(program_name);
    free(filename);

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <address> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int socket = get_connect_socket(argv[1], argv[2]);
    if (socket == -1) {
        fprintf(stderr, "Failed to connect to server.\n");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Connected to server.\n");

    char* command = calloc(COMMAND_BUF_LEN, sizeof(char));

    while (1) {
        fprintf(stdout, "Enter a command: ");
        char *ret = fgets(command, COMMAND_BUF_LEN, stdin);
        if (ret == NULL) {
            fprintf(stderr, "Error: fgets\n");
            break;
        }

        size_t n = strlen(command);
        if (n == 1 && command[0] == '\n') {
            continue;
        }

        handle_command(socket, command);

        memset(command, 0, n);
    }
    close(socket);
    free(command);
}
