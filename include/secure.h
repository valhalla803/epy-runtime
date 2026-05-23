#ifndef SECURE_H
#define SECURE_H

#include <stddef.h>

void secure_memzero(
    void* ptr,
    size_t size
);

#endif