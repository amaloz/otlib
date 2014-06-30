#include "crypto.h"

#include <openssl/sha.h>
#include <string.h>

#include "utils.h"

/*
 * Implementation (heavily) inspired by computePermutation() function found at
 * http://daimi.au.dk/~jot2re/cuda/resources/code2.zip : src/OT/protocols.c,
 * which implements Knuth's "Algorithm P".
 */
int
random_permutation(unsigned int *array, unsigned int size,
                   unsigned int *sorted, unsigned int seed)
{
    unsigned int *tmp;

    tmp = (unsigned int *) pymalloc(sizeof(unsigned int) * size);
    if (tmp == NULL)
        return FAILURE;

    srand(seed);

    /* Initialize identity permutation */
    for (unsigned int i = 0; i < size; ++i) {
        tmp[i] = i;
    }

    /* Permute the array */
    for (unsigned int i = size - 1; i >= 1; --i) {
        unsigned int j, tmpi, tmpj;

        j = (unsigned int) rand() % (i + 1);
        tmpi = tmp[i];
        tmpj = tmp[j];
        tmp[i] = tmpj;
        tmp[j] = tmpi;
    }

    /* Select pairs */
    for (unsigned int i = 0; i < size; i += 2) {
        array[tmp[i]] = tmp[i + 1];
        array[tmp[i + 1]] = tmp[i];
        if (sorted) {
            if (tmp[i] < tmp[i + 1]) {
                sorted[i / 2] = tmp[i];
            } else {
                sorted[i / 2] = tmp[i + 1];
            }
        }
    }

    free(tmp);

    return SUCCESS;
}

void
sha1_hash(char *output, size_t outputlen, int counter,
          unsigned char *hash, size_t hashlen)
{
    unsigned int length = 0;

    while (length < outputlen) {
        SHA_CTX c;
        int n;

        (void) SHA1_Init(&c);
        if (length == 0) {
            (void) SHA1_Update(&c, &counter, sizeof counter);
            (void) SHA1_Update(&c, hash, hashlen);
            (void) SHA1_Final(hash, &c);
        } else {
            (void) SHA1_Update(&c, output + length - hashlen, hashlen);
            (void) SHA1_Final(hash, &c);
        }
        n = MIN(outputlen - length, hashlen);
        (void) memcpy(output + length, hash, n);
        length += n;
    }
}

void
xorarray(unsigned char *a, const size_t alen,
         const unsigned char *b, const size_t blen)
{
    size_t i;

    assert(alen >= blen);
    for (i = 0; i < blen; ++i) {
        a[i] ^= b[i];
    }
}

