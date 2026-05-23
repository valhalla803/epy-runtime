#ifndef BUNDLE_H
#define BUNDLE_H

#include <stdint.h>

#define EPYB_MAGIC "EPYB"

typedef struct {

    char magic[4];

    uint32_t module_count;

    uint8_t bundle_salt[32];

    uint8_t build_seed[32];

    uint8_t signature[64];

} EPYBundleHeader;

typedef struct {

    char name[64];

    uint32_t offset;

    uint32_t size;

    uint8_t module_hash[32];

} EPYModuleEntry;

int epy_bundle_open(
    const char* filename
);

int epy_bundle_has_module(
    const char* module_name
);

unsigned char* epy_bundle_read_module(
    const char* module_name,
    uint32_t* out_size,
    EPYModuleEntry* out_entry
);

EPYBundleHeader* epy_bundle_get_header();

int epy_bundle_verify_signature(
    const char* filename
);

void epy_bundle_close();

#endif