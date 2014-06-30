#include "crypto.h"

#include <openssl/sha.h>
#include <string.h>

#include "utils.h"

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
