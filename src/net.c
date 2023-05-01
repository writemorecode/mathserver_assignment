#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "../include/net.h"

char *read_full(int fd, size_t length)
{
    char *buffer = calloc(length, sizeof(char));
    if (buffer == NULL)
    {
        return NULL;
    }

    size_t read_total = 0;
    ssize_t read_ret;
    while (read_total < length)
    {
        read_ret = read(fd, buffer + read_total, length - read_total);
        if (read_ret == -1)
        {
            perror("read");
            break;
        }
        if (read_ret == 0)
        {
            break;
        }
        read_total += (size_t) read_ret;
    }

    return buffer;
}

int write_full(int fd, char *buffer, size_t length)
{
    size_t write_total = 0;
    ssize_t write_ret;
    while (write_total < length)
    {
        write_ret = write(fd, buffer + write_total, length - write_total);
        if (write_ret == -1)
        {
            perror("write");
            break;
        }
        if (write_ret == 0)
        {
            break;
        }
        write_total += (size_t) write_ret;
    }

    return write_total;
}