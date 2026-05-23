#include "../include/crypto.h"

#include <openssl/evp.h>

int epy_decrypt(
    const unsigned char* ciphertext,
    int ciphertext_len,

    const unsigned char* key,
    const unsigned char* nonce,
    const unsigned char* tag,

    unsigned char* plaintext
) {
    EVP_CIPHER_CTX* ctx =
        EVP_CIPHER_CTX_new();

    if (!ctx) {
        return -1;
    }

    int len = 0;
    int plaintext_len = 0;

    if (
        EVP_DecryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            NULL,
            NULL,
            NULL
        ) != 1
    ) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_ctrl(
        ctx,
        EVP_CTRL_GCM_SET_IVLEN,
        12,
        NULL
    );

    EVP_DecryptInit_ex(
        ctx,
        NULL,
        NULL,
        key,
        nonce
    );

    EVP_DecryptUpdate(
        ctx,
        plaintext,
        &len,
        ciphertext,
        ciphertext_len
    );

    plaintext_len = len;

    EVP_CIPHER_CTX_ctrl(
        ctx,
        EVP_CTRL_GCM_SET_TAG,
        16,
        (void*)tag
    );

    int ret =
        EVP_DecryptFinal_ex(
            ctx,
            plaintext + len,
            &len
        );

    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        return -1;
    }

    plaintext_len += len;

    return plaintext_len;
}