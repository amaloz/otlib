#ifndef __OTLIB_UTILS_H__
#define __OTLIB_UTILS_H__

#include <gmp.h>

void *
pymalloc(size_t size);

int
pysend(int socket, const void *buffer, size_t length, int flags);

int
pyrecv(int socket, void *buffer, size_t length, int flags);

void
xorarray(char *a, const size_t alen, const char *b, const size_t blen);

void
mpz_to_array(char *buf, const mpz_t n, const size_t buflen);

void
array_to_mpz(mpz_t out, const char *buf, const size_t buflen);

#endif
