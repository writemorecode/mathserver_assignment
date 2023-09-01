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

#include "../include/command_handler.h"
#include "../include/epoll_handler.h"
#include "../include/fork_handler.h"
#include "../include/net.h"
#include "../include/pfd_array.h"
#include "../include/poll_handler.h"
#include "../include/socket.h"
#include "../include/string_array.h"
#include "../include/string_utils.h"

#define GNU_SOURCE

#define BACKLOG 10
#define BUF_LEN 4096
#define COMMAND_LEN 128
#define TIMEOUT 30 * 1000

enum Strategy {
    FORK,
    POLL,
    EPOLL
};

void run(enum Strategy strategy, char* port)
{
    int server_socket = get_listen_socket(port);

    if (server_socket == -1) {
        fprintf(stderr, "Failed to create socket.\n");
        return;
    }

    fprintf(stdout, "Listening on port %s\n", port);

    if (strategy == FORK) {
        fork_strategy(server_socket);
    } else if (strategy == POLL) {
        poll_strategy(server_socket);
    } else if (strategy == EPOLL) {
        epoll_strategy(server_socket);
    } else {
        fprintf(stderr, "Error: Invalid concurrency option.\n");
        return;
    }
}

void print_usage()
{
    printf("Mathserver Options\n");
    printf("-p\t\tPort to listen on.\n");
    printf("-d\t\tRun server as daemon.\n");
    printf("-s fork\t\tConcurrency using fork.\n");
    printf("-s poll\t\tConcurrency using poll.\n");
    printf("-s epoll\tConcurrency using epoll.\n");
}

int main(int argc, char** argv)
{
    int opt;
    char* port = "5000";
    bool daemon = false;
    enum Strategy strategy;
    strategy = EPOLL;

    while ((opt = getopt(argc, argv, "hdp:s:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            return 0;
        case 'd':
            daemon = true;
            break;
        case 'p':
            port = optarg;
            break;
        case 's':
            if (strcmp(optarg, "fork") == 0) {
                strategy = FORK;
            } else if (strcmp(optarg, "poll") == 0) {
                strategy = POLL;
            } else if (strcmp(optarg, "epoll") == 0) {
                strategy = EPOLL;
            }
        default:
            break;
        }
    }

    int status;
    if (daemon == false) {
        run(strategy, port);
    } else {
        pid_t pid_child = fork();
        if (pid_child == 0) {
            // Child
            pid_t pid_grandchild = fork();
            if (pid_grandchild == 0) {
                // Grandchild
                printf("PID: %d\n", getppid());
                run(strategy, port);
            }
            exit(EXIT_SUCCESS);
        } else {
            waitpid(pid_child, &status, 0);
        }
    }
}
