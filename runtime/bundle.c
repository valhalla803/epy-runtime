#include "../include/bundle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/err.h>

static FILE* bundle_fp = NULL;

static EPYBundleHeader bundle_header;

static EPYModuleEntry* module_table = NULL;

static const unsigned char EPY_PUBLIC_KEY[32] = {
    0xd7,0x5a,0x98,0x01,
    0x82,0xb1,0x0a,0xb7,
    0xd5,0x4b,0xfe,0xd3,
    0xc9,0x64,0x07,0x3a,
    0x0e,0xe1,0x72,0xf3,
    0xda,0xa6,0x23,0x25,
    0xaf,0x02,0x1a,0x68,
    0xf7,0x07,0x51,0x1a
};

EPYBundleHeader* epy_bundle_get_header() {
    return &bundle_header;
}

int epy_bundle_verify_signature(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    if (filesize <= 64) {
        fclose(fp);
        return 0;
    }

    unsigned char* buffer = malloc(filesize);
    if (!buffer) {
        fclose(fp);
        return 0;
    }

    if (fread(buffer, 1, filesize, fp) != (size_t)filesize) {
        free(buffer);
        fclose(fp);
        return 0;
    }

    fclose(fp);

    size_t data_len = (size_t)filesize - 64;
    unsigned char* signature = buffer + data_len;

    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519,
        NULL,
        EPY_PUBLIC_KEY,
        32
    );

    if (!pkey) {
        free(buffer);
        return 0;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        free(buffer);
        return 0;
    }

    if (EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        free(buffer);
        return 0;
    }

    int ok = EVP_DigestVerify(
        ctx,
        signature,
        64,
        buffer,
        data_len
    );

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    free(buffer);

    return ok == 1;
}

int epy_bundle_open(
    const char* filename
) {
    if (
        !epy_bundle_verify_signature(
            filename
        )
    ) {
        return 0;
    }

    bundle_fp = fopen(filename, "rb");

    if (!bundle_fp) {
        return 0;
    }

    if (
        fread(
            &bundle_header,
            sizeof(bundle_header),
            1,
            bundle_fp
        ) != 1
    ) {
        return 0;
    }

    if (
        memcmp(bundle_header.magic, EPYB_MAGIC, 4) != 0
    ) {
        return 0;
    }

    module_table = malloc(
        sizeof(EPYModuleEntry) *
        bundle_header.module_count
    );

    if (!module_table) {
        return 0;
    }

    if (
        fread(
            module_table,
            sizeof(EPYModuleEntry),
            bundle_header.module_count,
            bundle_fp
        ) != bundle_header.module_count
    ) {
        return 0;
    }

    return 1;
}

int epy_bundle_has_module(
    const char* module_name
) {
    for (
        uint32_t i = 0;
        i < bundle_header.module_count;
        i++
    ) {
        if (
            strcmp(
                module_table[i].name,
                module_name
            ) == 0
        ) {
            return 1;
        }
    }

    return 0;
}

unsigned char* epy_bundle_read_module(
    const char* module_name,
    uint32_t* out_size,
    EPYModuleEntry* out_entry
) {
    for (
        uint32_t i = 0;
        i < bundle_header.module_count;
        i++
    ) {
        if (
            strcmp(
                module_table[i].name,
                module_name
            ) == 0
        ) {
            unsigned char* buffer =
                malloc(module_table[i].size);

            if (!buffer) {
                return NULL;
            }

            fseek(
                bundle_fp,
                module_table[i].offset,
                SEEK_SET
            );

            if (
                fread(
                    buffer,
                    1,
                    module_table[i].size,
                    bundle_fp
                ) != module_table[i].size
            ) {
                free(buffer);
                return NULL;
            }

            *out_size = module_table[i].size;

            memcpy(
                out_entry,
                &module_table[i],
                sizeof(EPYModuleEntry)
            );

            return buffer;
        }
    }

    return NULL;
}