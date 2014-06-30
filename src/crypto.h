#ifndef __OTLIB_CRYPTO_H__
#define __OTLIB_CRYPTO_H__

#include <stdlib.h>

void
sha1_hash(char *output, size_t outputlen, int counter,
          unsigned char *hash, size_t hashlen);

#endif
