#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../include/pfd_array.h"
#include "../include/string_array.h"
#include "../include/string_utils.h"

#define GNU_SOURCE

#define BACKLOG 10
#define BUF_LEN 4096
#define TIMEOUT 30 * 1000

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

#define EXIT_SHUTDOWN 2

int get_listen_socket(char *port)
{
    int yes = 1;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    int status = getaddrinfo(NULL, port, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    int listen_socket;
    for (p = servinfo; p != NULL; p = servinfo->ai_next)
    {
        listen_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_socket == -1)
        {
            continue;
        }

        status = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(int));
        if (status == -1)
        {
            continue;
        }

        status = bind(listen_socket, p->ai_addr, p->ai_addrlen);
        if (status == -1)
        {
            close(listen_socket);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        // Failed to bind a listen_socket
        fprintf(stderr, "Failed to bind to a listen_socket.\n");
        return -1;
    }

    status = listen(listen_socket, BACKLOG);
    if (status == -1)
    {
        perror("listen");
        return -1;
    }

    return listen_socket;
}

int handle_command(int socket, char *buffer)
{
    char *program_name = get_program_name(buffer);
    printf("Recieved command: '%s'\n", buffer);
    printf("Program name: '%s'\n", program_name);
    struct string_array *args = string_array_from_string(buffer, " ");

    if (strcmp(program_name, "kmeanspar") == 0)
    {

        size_t input_data_size;
        ssize_t recv_ret = recv(socket, &input_data_size, sizeof(input_data_size), 0);
        if (recv_ret <= 0)
        {
            free(program_name);
            string_array_free(args);
            fprintf(stderr, "Error: Client failed to send size of input data.\n");
            return EXIT_FAILURE;
        }

        char *input_data_buffer = calloc(input_data_size, sizeof(char));
        if (input_data_buffer == NULL)
        {
            free(program_name);
            perror("calloc");
            return EXIT_FAILURE;
        }

        size_t recv_total = 0;
        while (recv_total < input_data_size)
        {
            recv_ret = recv(socket, input_data_buffer + recv_total, BUF_LEN, 0);
            if (recv_ret == -1)
            {
                free(program_name);
                perror("recv");
                break;
            }
            if (recv_ret == 0)
            {
                break;
            }
            recv_total += recv_ret;
        }

        char template[] = "kmeans-data-XXXXXX";
        int temp_fd = mkstemp(template);
        if (temp_fd == -1)
        {
            free(program_name);
            perror("mkstemp");
            return EXIT_FAILURE;
        }
        FILE *temp_fp = fdopen(temp_fd, "w");
        if (temp_fp == NULL)
        {
            perror("fdopen");
            free(program_name);
            free(input_data_buffer);
            string_array_free(args);
        }
        size_t write_total = 0;
        size_t write_ret;
        while (write_total < input_data_size)
        {
            write_ret = fwrite(input_data_buffer + write_total, sizeof(char), input_data_size - write_total, temp_fp);
            if (write_ret == 0)
            {
                break;
            }
            write_total += write_ret;
        }
        fclose(temp_fp);
        free(input_data_buffer);

        for(size_t i = 0; i < args->size; i++)
        {
            if(strcmp(args->data[i], "-f") == 0 && args->size > i + 1)
            {
                free(args->data[i+1]);
                args->data[i+1] = program_name;
            }
        }
    }

    free(program_name);    

    int pipefd[2];
    int status = pipe(pipefd);
    if (status == -1)
    {
        perror("pipe");
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (pid == 0)
    {
        close(pipefd[PIPE_READ_END]);
        dup2(pipefd[PIPE_WRITE_END], STDOUT_FILENO);
        close(pipefd[PIPE_WRITE_END]);

        int ret = execvp(args->data[0], args->data);
        string_array_free(args);
        if (ret == -1)
        {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            printf("DEBUG: Child exited with status %d\n", WEXITSTATUS(status));
        }
    }

    close(pipefd[PIPE_WRITE_END]);

    string_array_free(args);

    char *solution_buffer = calloc(BUF_LEN, sizeof(char));
    size_t solution_buffer_len = BUF_LEN;
    size_t read_total = 0;
    ssize_t read_ret;

    while (1)
    {
        if (read_total >= solution_buffer_len)
        {
            char *ret = realloc(solution_buffer, solution_buffer_len * 2);
            if (ret == NULL)
            {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            else
            {
                solution_buffer = ret;
            }
            solution_buffer_len *= 2;
        }
        read_ret = read(pipefd[PIPE_READ_END], solution_buffer + read_total, BUF_LEN);
        if (read_ret == -1)
        {
            perror("read");
            break;
        }
        if (read_ret == 0)
        {
            break;
        }
        read_total += read_ret;
    }

    // Send size of solution buffer to client
    ssize_t send_ret = send(socket, &read_total, sizeof(size_t), 0);

    // Send solution data to client
    ssize_t send_total = 0;
    do
    {
        send_ret = send(socket, solution_buffer + send_total, read_total - send_total, 0);
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
    } while (send_total < read_total);

    free(solution_buffer);

    return 0;
}

int handle_client(int socket)
{
    int ret = 0;
    while (1)
    {
        char *buffer = calloc(BUF_LEN, sizeof(char));
        ssize_t recv_ret = recv(socket, buffer, BUF_LEN, 0);
        if (recv_ret == 0)
        {
            fprintf(stdout, "Client closed connection to server.\n");
            break;
        }
        else if (recv_ret == -1)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        // The shortest command that can be sent to the server is "quit".
        // Commands shorter than four characters shall be ignored.
        if(recv_ret < 4)
        {
            free(buffer);
            continue;
        }

        if (recv_ret > 4 && strncmp("quit", buffer, 4) == 0)
        {
            fprintf(stdout, "Client quit.\n");
            free(buffer);
            return EXIT_SUCCESS;
        }

        if (recv_ret > 8 && strncmp("shutdown", buffer, 8) == 0)
        {
            fprintf(stdout, "Shutting down server.\n");
            free(buffer);
            return EXIT_SHUTDOWN;
        }

        handle_command(socket, buffer);

        free(buffer);
    }
    close(socket);
    return ret;
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

void poll_strategy(int server_socket)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    struct pfd_array *arr = pfd_array_new(2);
    pfd_array_insert(arr, server_socket);

    while (1)
    {
        int poll_count = poll(arr->data, arr->count, TIMEOUT);
        if (poll_count == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < arr->count; i++)
        {
            if (arr->data[i].revents & POLLIN)
            {
                if (arr->data[i].fd == server_socket)
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
                    int ret = handle_client(arr->data[i].fd);
                    pfd_array_remove(arr, i);
                }
            }
        }
    }

    pfd_array_free(arr);
}

enum Strategy
{
    FORK,
    MUXBASIC,
    MUXSCALE
};

void run(enum Strategy strategy, char *port)
{
    int server_socket = get_listen_socket(port);

    if (server_socket == -1)
    {
        fprintf(stderr, "Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Listening on port %s\n", port);

    if (strategy == FORK)
    {
        fork_strategy(server_socket);
    }
    else if (strategy == MUXBASIC)
    {
        poll_strategy(server_socket);
    }
}

int main(int argc, char **argv)
{
    int opt;
    char *port = "5000";
    bool daemon = false;
    enum Strategy strategy;
    strategy = MUXBASIC;

    while ((opt = getopt(argc, argv, "hdp:s:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Mathserver Options\n");
            printf("-p\t\tPort to listen on.\n");
            printf("-d\t\tRun server as daemon.\n");
            printf("-s fork\t\tConcurrency using fork.\n");
            printf("-s muxbasic\tConcurrency using poll.\n");
            printf("-s muxscale\tConcurrency using epoll.\n");
            return 0;
        case 'd':
            daemon = true;
            break;
        case 'p':
            port = optarg;
            break;
        case 's':
            if (strcmp(optarg, "fork") == 0)
            {
                strategy = FORK;
            }
            else if (strcmp(optarg, "muxbasic") == 0)
            {
                strategy = MUXBASIC;
            }
            else if (strcmp(optarg, "muxscale") == 0)
            {
                strategy = MUXSCALE;
            }
        default:
            break;
        }
    }

    int status;
    if (daemon == false)
    {
        run(strategy, port);
    }
    else
    {
        pid_t pid_child = fork();
        if (pid_child == 0)
        {
            // Child
            pid_t pid_grandchild = fork();
            if (pid_grandchild == 0)
            {
                // Grandchild
                printf("PID: %d\n", getppid());
                run(strategy, port);
            }
            exit(EXIT_SUCCESS);
        }
        else
        {
            waitpid(pid_child, &status, 0);
        }
    }
}
