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
