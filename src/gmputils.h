#ifndef __OTLIB_GMPUTILS_H__
#define __OTLIB_GMPUTILS_H__

#include <gmp.h>

#define FIELD_SIZE 128          /* the field size in bytes */

struct params {
    mpz_t p;
    mpz_t g;
    mpz_t q;
    gmp_randstate_t rnd;
};

void
random_element(mpz_t out, struct params *p);

void
mpz_to_array(char *buf, const mpz_t n, const size_t buflen);

void
array_to_mpz(mpz_t out, const char *buf, const size_t buflen);

void
find_generator(mpz_t g, struct params *params);

#endif
