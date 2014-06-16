#include "gmputils.h"

#include <string.h>

void
random_element(mpz_t out, struct params *p)
{
    mpz_urandomb(out, p->rnd, FIELD_SIZE * 8);
    mpz_mod(out, out, p->p);
}

void
mpz_to_array(char *buf, const mpz_t n, const size_t buflen)
{
    size_t len = 0;
    memset(buf, '\0', FIELD_SIZE);
    (void) mpz_export(buf, &len, -1, sizeof(char), 0, 0, n);
}

void
array_to_mpz(mpz_t out, const char *buf, const size_t buflen)
{
    mpz_import(out, buflen, -1, sizeof(char), 0, 0, buf);
}

void
find_generator(mpz_t g, struct params *params)
{
    mpz_t exp, pminusone, tmp;

    mpz_inits(exp, pminusone, tmp, NULL);
    mpz_sub_ui(pminusone, params->p, 1);
    mpz_div_ui(exp, pminusone, 2);
    do {
        random_element(g, params);
        /* XXX: this assumes q is a safe prime */
        mpz_powm(tmp, g, exp, params->p);
    } while (mpz_cmp(tmp, pminusone) != 0);

    mpz_clears(exp, pminusone, tmp, NULL);
}
