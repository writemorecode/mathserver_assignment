/*
This code handles the array of FDs used by poll()
*/

#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/pfd_array.h"

struct pfd_array* pfd_array_new()
{
    struct pfd_array* p = calloc(1, sizeof(struct pfd_array));
    p->data = calloc(1, sizeof(struct pollfd));
    p->capacity = 1;
    p->count = 0;
    return p;
}

void pfd_array_free(struct pfd_array* p)
{
    free(p->data);
    free(p);
}

void pfd_array_insert(struct pfd_array* p, int fd)
{
    if (p->count == p->capacity) {
        p->capacity *= 2;
        void* ret = realloc(p->data, sizeof(struct pollfd) * p->capacity);
        if (ret == NULL) {
            perror("realloc");
            return;
        } else {
            p->data = ret;
        }
    }

    p->data[p->count].fd = fd;
    p->data[p->count].events = POLLIN;
    p->count += 1;
}

void pfd_array_remove(struct pfd_array* p, size_t index)
{
    if (index >= p->capacity) {
        return;
    }
    p->data[index].fd = -1;
    p->data[index].events = 0;
    p->data[index].revents = 0;
    p->count -= 1;
}
