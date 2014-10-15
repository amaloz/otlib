#ifndef __OTLIB_CRYPTO_H__
#define __OTLIB_CRYPTO_H__

#include <stdlib.h>
#include <openssl/evp.h>

int
random_permutation(unsigned int *array, unsigned int size,
                   unsigned int *sorted, unsigned int seed);

void
sha1_hash(char *output, size_t outputlen, int counter,
          const unsigned char *hash, size_t hashlen);

void
xorarray(unsigned char *a, const size_t alen,
         const unsigned char *b, const size_t blen);

int
aes_init(unsigned char *keydata, int keydatalen,
         EVP_CIPHER_CTX *enc, EVP_CIPHER_CTX *dec);

unsigned char *
aes_encrypt(EVP_CIPHER_CTX *ctx, unsigned char *ptxt, int *len);

unsigned char *
aes_decrypt(EVP_CIPHER_CTX *ctx, unsigned char *ctxt, int *len);

#endif
