#include "../include/epy.h"
#include "../include/crypto.h"
#include "../include/bundle.h"
#include "../include/secure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/kdf.h>

#include <zlib.h>

static unsigned char runtime_secret_1[32] = {
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x10,
    0x21,0x32,0x43,0x54,0x65,0x76,0x87,0x98,
    0xa9,0xba,0xcb,0xdc,0xed,0xfe,0x0f,0x1a
};

static unsigned char runtime_secret_2[32] = {
    0x91,0x82,0x73,0x64,0x55,0x46,0x37,0x28,
    0x19,0x0a,0xfb,0xec,0xdd,0xce,0xbf,0xa0,
    0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x89,
    0x9a,0xab,0xbc,0xcd,0xde,0xef,0xf1,0x02
};

static void derive_master_key(
    EPYBundleHeader* bundle_header,
    unsigned char* out_key
) {
    EVP_MD_CTX* ctx =
        EVP_MD_CTX_new();

    EVP_DigestInit_ex(
        ctx,
        EVP_sha256(),
        NULL
    );

    EVP_DigestUpdate(
        ctx,
        runtime_secret_1,
        32
    );

    EVP_DigestUpdate(
        ctx,
        runtime_secret_2,
        32
    );

    EVP_DigestUpdate(
        ctx,
        bundle_header->bundle_salt,
        32
    );

    EVP_DigestUpdate(
        ctx,
        bundle_header->build_seed,
        32
    );

    unsigned int len = 32;

    EVP_DigestFinal_ex(
        ctx,
        out_key,
        &len
    );

    EVP_MD_CTX_free(ctx);
}

static void derive_module_key(
    const unsigned char* master_key,
    EPYModuleEntry* entry,
    unsigned char* out_key
) {
    EVP_KDF* kdf =
        EVP_KDF_fetch(
            NULL,
            "HKDF",
            NULL
        );

    EVP_KDF_CTX* kctx =
        EVP_KDF_CTX_new(kdf);

    unsigned char info[96];

    memcpy(
        info,
        entry->module_hash,
        32
    );

    memcpy(
        info + 32,
        entry->name,
        strlen(entry->name)
    );

    size_t info_len =
        32 + strlen(entry->name);

    OSSL_PARAM params[4];

    params[0] =
        OSSL_PARAM_construct_utf8_string(
            "digest",
            "SHA256",
            0
        );

    params[1] =
        OSSL_PARAM_construct_octet_string(
            "key",
            (void*)master_key,
            32
        );

    params[2] =
        OSSL_PARAM_construct_octet_string(
            "info",
            info,
            info_len
        );

    params[3] =
        OSSL_PARAM_construct_end();

    EVP_KDF_derive(
        kctx,
        out_key,
        32,
        params
    );

    secure_memzero(
        info,
        sizeof(info)
    );

    EVP_KDF_CTX_free(kctx);

    EVP_KDF_free(kdf);
}

unsigned char* epy_decrypt_module(
    const unsigned char* encrypted_data,
    EPYHeader* header,
    EPYBundleHeader* bundle_header,
    EPYModuleEntry* module_entry,
    uint32_t* out_size
) {
    unsigned char master_key[32];

    derive_master_key(
        bundle_header,
        master_key
    );

    unsigned char module_key[32];

    derive_module_key(
        master_key,
        module_entry,
        module_key
    );

    unsigned char* compressed =
        malloc(header->raw_size);

    if (!compressed) {

        secure_memzero(
            master_key,
            32
        );

        secure_memzero(
            module_key,
            32
        );

        return NULL;
    }

    int decrypted_size =
        epy_decrypt(
            encrypted_data,
            header->encrypted_size,

            module_key,
            header->nonce,
            header->tag,

            compressed
        );

    secure_memzero(
        master_key,
        32
    );

    secure_memzero(
        module_key,
        32
    );

    if (decrypted_size < 0) {

        secure_memzero(
            compressed,
            header->raw_size
        );

        free(compressed);

        return NULL;
    }

    unsigned char* raw =
        malloc(header->raw_size);

    if (!raw) {

        secure_memzero(
            compressed,
            header->raw_size
        );

        free(compressed);

        return NULL;
    }

    uLongf decompressed_size =
        header->raw_size;

    if (
        uncompress(
            raw,
            &decompressed_size,
            compressed,
            decrypted_size
        ) != Z_OK
    ) {

        secure_memzero(
            compressed,
            header->raw_size
        );

        secure_memzero(
            raw,
            header->raw_size
        );

        free(compressed);

        free(raw);

        return NULL;
    }

    secure_memzero(
        compressed,
        header->raw_size
    );

    free(compressed);

    *out_size = decompressed_size;

    return raw;
}