#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTEN_BACKLOG 128

int set_socket_nonblocking(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

int get_listen_socket(char* port)
{
    int yes = 1;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    int status = getaddrinfo(NULL, port, &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    int listen_socket;
    for (p = servinfo; p != NULL; p = servinfo->ai_next) {
        listen_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_socket == -1) {
            continue;
        }

        status = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(int));
        if (status == -1) {
            continue;
        }

        status = bind(listen_socket, p->ai_addr, p->ai_addrlen);
        if (status == -1) {
            close(listen_socket);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        // Failed to bind a listen_socket
        fprintf(stderr, "Failed to bind to a listen_socket.\n");
        return -1;
    }

    status = listen(listen_socket, LISTEN_BACKLOG);
    if (status == -1) {
        perror("listen");
        return -1;
    }

    return listen_socket;
}
