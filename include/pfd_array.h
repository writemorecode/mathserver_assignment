#ifndef PFD_ARRAY_H
#define PFD_ARRAY_H

#include <stddef.h>

struct pfd_array
{
    struct pollfd *data;
    size_t count, capacity;
};

struct pfd_array *pfd_array_new(size_t capacity_);
void pfd_array_free(struct pfd_array *p);
void pfd_array_insert(struct pfd_array *p, int fd);
void pfd_array_remove(struct pfd_array *p, size_t index);

#endif
