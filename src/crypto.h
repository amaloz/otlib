#ifndef __OTLIB_CRYPTO_H__
#define __OTLIB_CRYPTO_H__

#include <stdlib.h>

int
random_permutation(unsigned int *array, unsigned int size,
                   unsigned int *sorted, unsigned int seed);

void
sha1_hash(char *output, size_t outputlen, int counter,
          unsigned char *hash, size_t hashlen);

void
xorarray(unsigned char *a, const size_t alen,
         const unsigned char *b, const size_t blen);

#endif
