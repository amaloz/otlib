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

PyObject *
state_init(PyObject *self, PyObject *args);

void
random_element(mpz_t out, struct params *p);

#endif
