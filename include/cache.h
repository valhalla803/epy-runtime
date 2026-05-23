#ifndef EPY_CACHE_H
#define EPY_CACHE_H

#include <Python.h>
#include <stdint.h>

typedef struct CacheNode {
    char name[64];
    PyObject* code_object;
    struct CacheNode* next;
} CacheNode;

void epy_cache_init();
PyObject* epy_cache_get(const char* name);
void epy_cache_set(const char* name, PyObject* code);
void epy_cache_clear();

#endif