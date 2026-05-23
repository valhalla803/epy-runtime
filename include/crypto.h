#ifndef CRYPTO_H
#define CRYPTO_H

int epy_decrypt(
    const unsigned char* ciphertext,
    int ciphertext_len,

    const unsigned char* key,
    const unsigned char* nonce,
    const unsigned char* tag,

    unsigned char* plaintext
);

#endif