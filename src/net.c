#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/net.h"

#define BUF_LEN 4096

// Allocates a buffer and reads from fd into buffer
// until exactly length bytes have been read.
char* read_full(int fd, size_t length)
{
    char* buffer = calloc(length, sizeof(char));
    if (buffer == NULL) {
        return NULL;
    }

    size_t read_total = 0;
    ssize_t read_ret;
    while (read_total < length) {
        read_ret = read(fd, buffer + read_total, length - read_total);
        if (read_ret == -1) {
            perror("read");
            break;
        }
        if (read_ret == 0) {
            break;
        }
        read_total += (size_t)read_ret;
    }

    return buffer;
}

// Writes exactly length bytes from buffer to fd.
int write_full(int fd, char* buffer, size_t length)
{
    size_t write_total = 0;
    ssize_t write_ret;
    while (write_total < length) {
        write_ret = write(fd, buffer + write_total, length - write_total);
        if (write_ret == -1) {
            perror("write");
            break;
        }
        if (write_ret == 0) {
            break;
        }
        write_total += (size_t)write_ret;
    }

    return write_total;
}

// Allocates a buffer, and reads from fd into this
// buffer until read returns zero.
// The number of bytes read from fd will be written to n.
char* read_all(int fd, size_t* n)
{
    size_t buffer_capacity = 4096;
    char *buffer = calloc(buffer_capacity, sizeof(char));
    if (buffer == NULL)
    {
        return NULL;
    }
    size_t read_total = 0;
    ssize_t read_ret = 0;

    while(1)
    {
        read_ret = read(fd, buffer + read_total, buffer_capacity - read_total);
        if (read_ret < 0)
        {
            free(buffer);
            perror("read");
            return NULL;
        }
        if (read_ret == 0) {
            *n = read_total;
            return buffer;
        }
        read_total += read_ret;

        if (read_total >= buffer_capacity) {
            buffer_capacity *= 2;
            char *p = realloc(buffer, buffer_capacity);
            if (p == NULL)
            {
                free(buffer);
                perror("realloc");
                return NULL;
            }
            buffer = p;
        }
    }
}
