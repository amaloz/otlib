#ifndef __OTLIB_STATE_H__
#define __OTLIB_STATE_H__

#define FIELD_SIZE 128          /* the field size in bytes */

#include <Python.h>
#include <gmp.h>

struct params {
    mpz_t p;
    mpz_t g;
    mpz_t q;
    gmp_randstate_t rnd;
};

struct state {
    struct params p;
    long length;
    int sockfd;
    int serverfd;
};

void
random_element(mpz_t out, struct params *p);

int
state_initialize(struct state *s, long length);

void
state_cleanup(struct state *s);

void
state_destructor(PyObject *self);

#endif
