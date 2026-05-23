#ifndef EPY_H
#define EPY_H

#include <Python.h>
#include <stdint.h>

#include "bundle.h"
#include "cache.h"

#define EPY_MAGIC "EPY\0"

typedef struct {
    char magic[4];

    uint16_t py_major;
    uint16_t py_minor;

    uint32_t raw_size;
    uint32_t encrypted_size;

    uint8_t nonce[12];
    uint8_t tag[16];

} EPYHeader;

unsigned char* epy_decrypt_module(
    const unsigned char* encrypted_data,
    EPYHeader* header,
    EPYBundleHeader* bundle_header,
    EPYModuleEntry* module_entry,
    uint32_t* out_size
);

PyObject* epy_import_module(
    const char* name
);

#endif