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
    char* buffer = calloc(BUF_LEN, sizeof(char));
    size_t buffer_len = BUF_LEN;
    size_t read_total = 0;
    ssize_t read_ret;

    // Read from read end of pipe until read returns zero.
    while (1) {
        if (read_total >= buffer_len) {
            char* ret = realloc(buffer, buffer_len * 2);
            if (ret == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            } else {
                buffer = ret;
            }
            buffer_len *= 2;
        }
        read_ret = read(fd, buffer + read_total, BUF_LEN);
        if (read_ret == -1) {
            perror("read");
            break;
        }
        if (read_ret == 0) {
            break;
        }
        read_total += read_ret;
    }
    *n = read_total;
    return buffer;
}
