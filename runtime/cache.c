#include "../include/cache.h"
#include "../include/secure.h"

#include <stdlib.h>
#include <string.h>

static CacheNode* head = NULL;

void epy_cache_init() {
    head = NULL;
}

PyObject* epy_cache_get(
    const char* name
) {
    CacheNode* cur = head;

    while (cur) {

        if (
            strcmp(cur->name, name) == 0
        ) {
            return cur->code_object;
        }

        cur = cur->next;
    }

    return NULL;
}

void epy_cache_set(
    const char* name,
    PyObject* code
) {
    if (!name || !code) {
        return;
    }

    CacheNode* node =
        malloc(sizeof(CacheNode));

    if (!node) {
        return;
    }

    memset(
        node,
        0,
        sizeof(CacheNode)
    );

    strncpy(
        node->name,
        name,
        63
    );

    node->name[63] = '\0';

    node->code_object = code;

    Py_INCREF(code);

    node->next = head;

    head = node;
}

void epy_cache_clear() {

    CacheNode* cur = head;

    while (cur) {

        CacheNode* next =
            cur->next;

        if (cur->code_object) {
            Py_DECREF(cur->code_object);
        }

        secure_memzero(
            cur,
            sizeof(CacheNode)
        );

        free(cur);

        cur = next;
    }

    head = NULL;
}