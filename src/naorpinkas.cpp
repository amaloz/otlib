#include "naorpinkas.h"

#include "log.h"
#include "net.h"
#include "state.h"
#include "utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>
#include <openssl/sha.h>

const char *tag = "OT-NP";

PyObject *
np_send(PyObject *self, PyObject *args)
{
    mpz_t r, gr, pk0;
    mpz_t *Cs = NULL, *Crs = NULL, *pks = NULL;
    char buf[FIELD_SIZE];
    PyObject *py_state, *py_msgs;
    long N, num_ots, err = 0;
    int maxlength;
    struct state *s;

    if (!PyArg_ParseTuple(args, "OOi", &py_state, &py_msgs, &maxlength))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_msgs)) == -1)
        return NULL;

    if ((N = PySequence_Length(PySequence_GetItem(py_msgs, 0))) == -1)
        return NULL;

    // check to make sure all OTs are 1-out-of-N OTs and all messages are of
    // length <= maxlength
    for (int j = 0; j < num_ots; ++j) {
        PyObject *tuple = PySequence_GetItem(py_msgs, j);
        if (PySequence_Length(tuple) != N) {
            PyErr_SetString(PyExc_TypeError, "unmatched input length");
            return NULL;
        }
        for (int i = 0; i < N; ++i) {
            if (PyString_Size(PySequence_GetItem(tuple, i)) > maxlength) {
                PyErr_SetString(PyExc_TypeError, "message > maxlength");
                return NULL;
            }
        }
    }

    mpz_inits(r, gr, pk0, NULL);

    Cs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL) {
        err = 1;
        goto cleanup;
    }
    Crs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Crs == NULL) {
        err = 1;
        goto cleanup;
    }
    pks = (mpz_t *) malloc(sizeof(mpz_t) * N);
    if (pks == NULL) {
        err = 1;
        goto cleanup;
    }
    
    // choose r \in_R Zq
    random_element(r, &s->p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "r = ", r);
    // compute g^r
    mpz_powm(gr, s->p.g, r, s->p.p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "gr = ", gr);

    for (int i = 0; i < N - 1; ++i) {
        mpz_inits(Cs[i], Crs[i], NULL);
        // choose C_i \in_R Zq
        random_element(Cs[i], &s->p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "Cs[i] = ", Cs[i]);
        // compute C_i^r
        mpz_powm(Crs[i], Cs[i], r, s->p.p);
    }
    for (int i = 0; i < N; ++i) {
        mpz_init(pks[i]);
    }

    // send g^r to receiver
    mpz_to_array(buf, gr, sizeof buf);
    if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    // send Cs to receiver
    for (int i = 0; i < N - 1; ++i) {
        mpz_to_array(buf, Cs[i], sizeof buf);
        if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
    }

    for (int j = 0; j < num_ots; ++j) {
        PyObject *py_input;

        py_input = PySequence_GetItem(py_msgs, j);
        // get pk0 from receiver
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
        array_to_mpz(pk0, buf, sizeof buf);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "pk0 = ", pk0);

        // compute pk0^r
        mpz_powm(pks[0], pk0, r, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "pks[0] = ", pks[0]);
        // compute pk_i^r
        for (int i = 0; i < N - 1; ++i) {
            (void) mpz_invert(pks[i + 1], pks[0], s->p.p);
            mpz_mul(pks[i + 1], pks[i + 1], Crs[i]);
            mpz_mod(pks[i + 1], pks[i + 1], s->p.p);
            logger_mpz(LOG_LEVEL_DEBUG, tag, "pks[i] = ", pks[i + 1]);
        }

        for (int i = 0; i < N; ++i) {
            SHA_CTX c;
            char hash[SHA_DIGEST_LENGTH];
            Py_ssize_t mlen;
            char *m;

            // FIXME: make this more flexible
            assert(maxlength <= (int) sizeof hash);

            mpz_to_array(buf, pks[i], sizeof buf);
            SHA1_Init(&c);
            SHA1_Update(&c, buf, sizeof buf);
            SHA1_Update(&c, &i, sizeof i);
            SHA1_Final((unsigned char *) hash, &c);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            assert(mlen <= maxlength);
            xorarray(hash, maxlength, m, mlen);
            if (pysend(s->sockfd, hash, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }
        }
    }

 cleanup:
    mpz_clears(r, gr, pk0, NULL);

    if (Cs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Cs[i]);
        free(Cs);
    }
    if (Crs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Crs[i]);
        free(Crs);
    }
    if (pks) {
        for (int i = 0; i < N; ++i)
            mpz_clear(pks[i]);
        free(pks);
    }

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
np_receive(PyObject *self, PyObject *args)
{
    mpz_t k, gr, PK0, PKs, PKsr;
    mpz_t *Cs = NULL;
    char buf[FIELD_SIZE];
    struct state *s;
    PyObject *py_state, *py_choices, *py_return = NULL;
    int num_ots, err = 0;
    int N, maxlength;

    if (!PyArg_ParseTuple(args, "OOii", &py_state, &py_choices, &N, &maxlength))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_choices)) == -1)
        return NULL;

    Cs = (mpz_t *) malloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL) {
        err = 1;
        goto cleanup;
    }

    mpz_inits(k, gr, PK0, PKs, PKsr, NULL);

    for (int i = 0; i < N - 1; ++i) {
        mpz_init(Cs[i]);
    }

    // get g^r from sender
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    array_to_mpz(gr, buf, sizeof buf);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "gr = ", gr);

    // get Cs from sender
    for (int i = 0; i < N - 1; ++i) {
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
        array_to_mpz(Cs[i], buf, sizeof buf);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "Cs[i] = ", Cs[i]);
    }

    py_return = PyTuple_New(num_ots);

    for (int j = 0; j < num_ots; ++j) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));
        // choose random k
        mpz_urandomb(k, s->p.rnd, FIELD_SIZE * 8);
        mpz_mod(k, k, s->p.q);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "k = ", k);
        // compute PKs = g^k
        mpz_powm(PKs, s->p.g, k, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "PKs = ", PKs);

        (void) mpz_invert(PK0, PKs, s->p.p);
        mpz_mul(PK0, PK0, Cs[0]);
        mpz_mod(PK0, PK0, s->p.p);
        mpz_set(PK0, choice == 0 ? PKs : PK0);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "PK0 = ", PK0);

        // send PK0 to sender

        mpz_to_array(buf, PK0, sizeof buf);
        if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }

        // compute (g^r)^k = PKs^r
        mpz_powm(PKsr, gr, k, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "PKsr = ", PKsr);

        for (int i = 0; i < N; ++i) {
            char h[SHA_DIGEST_LENGTH], out[SHA_DIGEST_LENGTH];
            SHA_CTX c;

            assert(maxlength <= (int) sizeof h);

            memset(h, '\0', sizeof h);
            // get H xor M0 from sender
            if (pyrecv(s->sockfd, h, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }
            mpz_to_array(buf, PKsr, sizeof buf);
            (void) SHA1_Init(&c);
            (void) SHA1_Update(&c, buf, sizeof buf);
            (void) SHA1_Update(&c, &i, sizeof i);
            (void) SHA1_Final((unsigned char *) out, &c);
            xorarray(out, maxlength, h, maxlength);
            memset(out + maxlength, '\0', sizeof out - maxlength);
            if (i == choice) {
                PyObject *str = PyString_FromString(out);
                if (str == NULL) {
                    err = 1;
                    goto cleanup;
                } else {
                    PyTuple_SetItem(py_return, j, str);
                }
            }
        }
    }

 cleanup:
    mpz_clears(k, gr, PK0, PKs, PKsr, NULL);

    if (Cs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Cs[i]);
        free(Cs);
    }

    // TODO: clean up py_return if error

    if (err)
        return NULL;
    else
        return py_return;
}
