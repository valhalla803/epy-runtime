#include "../include/secure.h"

#include <stddef.h>

void secure_memzero(
    void* ptr,
    size_t size
) {
    volatile unsigned char* p =
        (volatile unsigned char*)ptr;

    while (size--) {
        *p++ = 0;
    }
}