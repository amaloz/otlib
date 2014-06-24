#include "gmputils.h"
#include "utils.h"

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

int
encode(mpz_t elem, const char *str, size_t strlen, const struct params *p)
{
    mpz_t exp, rop;

    mpz_inits(exp, rop, NULL);

    if (strlen >= FIELD_SIZE)
        return FAILURE;
    mpz_import(elem, strlen, -1, sizeof str[0], 0, 0, str);
    mpz_add_ui(elem, elem, 1);

    /* compute (p - 1) / 2 */
    mpz_sub_ui(exp, p->p, 1);
    mpz_divexact_ui(exp, exp, 2);
    /* compute elem ^ exp mod p */
    mpz_powm(rop, elem, exp, p->p);

    if (mpz_cmp_ui(rop, 1) != 0) {
        mpz_neg(elem, elem);
        mpz_mod(elem, elem, p->p);
    }

    mpz_clears(exp, rop, NULL);

    return SUCCESS;
}

char *
decode(const mpz_t elem, const struct params *p)
{
    mpz_t tmp;
    char *out;
    size_t count = 0;

    mpz_init(tmp);

    if (mpz_cmp(elem, p->q) <= 0) {
        mpz_sub_ui(tmp, elem, 1);
    } else {
        mpz_neg(tmp, elem);
        mpz_mod(tmp, tmp, p->p);
        mpz_sub_ui(tmp, tmp, 1);
    }

    out = (char *) mpz_export(NULL, &count, -1, sizeof(char), 0, 0, tmp);

    mpz_clear(tmp);

    return out;
}
