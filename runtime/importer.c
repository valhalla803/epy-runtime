#include "../include/epy.h"
#include "../include/bundle.h"
#include "../include/cache.h"
#include "../include/secure.h"

#include <Python.h>
#include <marshal.h>

#include <stdlib.h>
#include <string.h>

extern unsigned char* epy_decrypt_module(
    const unsigned char* encrypted_data,
    EPYHeader* header,
    EPYBundleHeader* bundle_header,
    EPYModuleEntry* module_entry,
    uint32_t* out_size
);

PyObject* epy_import_module(
    const char* module_name
) {
    if (!module_name) {
        return NULL;
    }

    PyObject* cached =
        epy_cache_get(module_name);

    if (cached) {
        Py_INCREF(cached);
        return cached;
    }

    uint32_t module_size = 0;

    EPYModuleEntry entry;

    unsigned char* module_data =
        epy_bundle_read_module(
            module_name,
            &module_size,
            &entry
        );

    if (!module_data) {
        return NULL;
    }

    EPYHeader header;

    memcpy(
        &header,
        module_data,
        sizeof(EPYHeader)
    );

    unsigned char* encrypted =
        module_data + sizeof(EPYHeader);

    EPYBundleHeader* bundle_header =
        epy_bundle_get_header();

    uint32_t raw_size = 0;

    unsigned char* raw =
        epy_decrypt_module(
            encrypted,
            &header,
            bundle_header,
            &entry,
            &raw_size
        );

    secure_memzero(
        module_data,
        module_size
    );

    free(module_data);

    if (!raw) {
        return NULL;
    }

    PyObject* code =
        PyMarshal_ReadObjectFromString(
            (const char*)raw,
            raw_size
        );

    secure_memzero(
        raw,
        raw_size
    );

    free(raw);

    if (!code) {
        PyErr_Print();
        return NULL;
    }

    PyObject* module =
        PyModule_New(module_name);

    if (!module) {
        Py_DECREF(code);
        return NULL;
    }

    PyObject* globals =
        PyModule_GetDict(module);

    PyObject* result =
        PyEval_EvalCode(
            code,
            globals,
            globals
        );

    Py_DECREF(code);

    if (!result) {
        PyErr_Print();
        Py_DECREF(module);
        return NULL;
    }

    Py_DECREF(result);

    epy_cache_set(
        module_name,
        module
    );

    Py_INCREF(module);

    return module;
}