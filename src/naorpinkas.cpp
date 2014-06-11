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

static void
build_hash(char *final, char *buf, int index, const int maxlength)
{
    int length = 0;
    while (length < maxlength) {
        SHA_CTX c;
        char hash[SHA_DIGEST_LENGTH];
        int n;

        (void) SHA1_Init(&c);
        if (length == 0) {
            (void) SHA1_Update(&c, buf, sizeof buf);
            (void) SHA1_Update(&c, &index, sizeof index);
            (void) SHA1_Final((unsigned char *) hash, &c);
        } else {
            (void) SHA1_Update(&c, final + length - sizeof hash, sizeof hash);
            (void) SHA1_Final((unsigned char *) hash, &c);
        }
        n = MIN(maxlength - length, (int) sizeof hash);
        (void) memcpy(final + length, hash, n);
        length += n;
    }
}

PyObject *
np_send(PyObject *self, PyObject *args)
{
    mpz_t r, gr, pk, pk0;
    mpz_t *Cs = NULL, *Crs = NULL, *pk0s = NULL;
    char buf[FIELD_SIZE], *msg = NULL;
    PyObject *py_state, *py_msgs;
    long N, num_ots, err = 0;
    int maxlength;
    struct state *s;
    double start, end;

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

    mpz_inits(r, gr, pk, pk0, NULL);

    msg = (char *) pymalloc(sizeof(char) * maxlength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }
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
    pk0s = (mpz_t *) pymalloc(sizeof(mpz_t) * num_ots);
    if (pk0s == NULL) {
        err = 1;
        goto cleanup;
    }

    for (int i = 0; i < num_ots; ++i) {
        mpz_init(pk0s[i]);
    }

    start = current_time();
    // choose r \in_R Zq
    random_element(r, &s->p);
    // compute g^r
    mpz_powm(gr, s->p.g, r, s->p.p);

    // choose C_i's \in_R Zq
    for (int i = 0; i < N - 1; ++i) {
        mpz_inits(Cs[i], Crs[i], NULL);
        random_element(Cs[i], &s->p);
    }
    end = current_time();
    fprintf(stderr, "COMPUTE: r, g^r, C_i: %f\n", end - start);

    start = current_time();
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
    end = current_time();
    fprintf(stderr, "SEND: g^r, Cs: %f\n", end - start);

    start = current_time();
    for (int i = 0; i < N - 1; ++i) {
        // compute C_i^r
        mpz_powm(Crs[i], Cs[i], r, s->p.p);
    }
    end = current_time();
    fprintf(stderr, "COMPUTE: C_i^r: %f\n", end - start);

    start = current_time();
    for (int j = 0; j < num_ots; ++j) {
        // get pk0 from receiver
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
        array_to_mpz(pk0s[j], buf, sizeof buf);
    }
    end = current_time();
    fprintf(stderr, "RECEIVE: pk0s: %f\n", end - start);

    for (int j = 0; j < num_ots; ++j) {
        PyObject *py_input;

        py_input = PySequence_GetItem(py_msgs, j);

        start = current_time();
        for (int i = 0; i < N; ++i) {
            Py_ssize_t mlen;
            char *m;

            if (i == 0) {
                // compute pk0^r
                mpz_powm(pk0, pk0s[j], r, s->p.p);
                mpz_to_array(buf, pk0, sizeof buf);
            } else {
                (void) mpz_invert(pk, pk0, s->p.p);
                mpz_mul(pk, pk, Crs[i - 1]);
                mpz_mod(pk, pk, s->p.p);
                mpz_to_array(buf, pk, sizeof buf);
            }
            build_hash(msg, buf, i, maxlength);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            assert(mlen <= maxlength);
            xorarray(msg, maxlength, m, mlen);
            if (pysend(s->sockfd, msg, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }
        }
        end = current_time();
        fprintf(stderr, "SEND: hashes: %f\n", end - start);
    }

 cleanup:
    mpz_clears(r, gr, pk, pk0, NULL);

    if (pk0s) {
        for (int i = 0; i < num_ots; ++i) {
            mpz_clear(pk0s[i]);
        }
        free(pk0s);
    }
    if (Crs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Crs[i]);
        free(Crs);
    }
    if (Cs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Cs[i]);
        free(Cs);
    }
    if (msg)
        free(msg);

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
np_receive(PyObject *self, PyObject *args)
{
    mpz_t gr, pk0, pks;
    mpz_t *Cs = NULL, *ks = NULL;
    char buf[FIELD_SIZE], *from = NULL, *msg = NULL;
    struct state *s;
    PyObject *py_state, *py_choices, *py_return = NULL;
    int num_ots, err = 0;
    int N, maxlength;
    double start, end;

    if (!PyArg_ParseTuple(args, "OOii", &py_state, &py_choices, &N, &maxlength))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_choices)) == -1)
        return NULL;

    mpz_inits(gr, pk0, pks, NULL);

    py_return = PyTuple_New(num_ots);

    msg = (char *) pymalloc(sizeof(char) * maxlength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }
    from = (char *) pymalloc(sizeof(char) * maxlength);
    if (from == NULL) {
        err = 1;
        goto cleanup;
    }
    Cs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL) {
        err = 1;
        goto cleanup;
    }
    for (int i = 0; i < N - 1; ++i) {
        mpz_init(Cs[i]);
    }
    ks = (mpz_t *) pymalloc(sizeof(mpz_t) * num_ots);
    if (ks == NULL) {
        err = 1;
        goto cleanup;
    }
    for (int j = 0; j < num_ots; ++j) {
        mpz_init(ks[j]);
    }

    start = current_time();
    // get g^r from sender
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    array_to_mpz(gr, buf, sizeof buf);
    end = current_time();
    fprintf(stderr, "RECEIVE: g^r: %f\n", end - start);

    start = current_time();
    // get Cs from sender
    for (int i = 0; i < N - 1; ++i) {
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
        array_to_mpz(Cs[i], buf, sizeof buf);
    }
    end = current_time();
    fprintf(stderr, "RECEIVE: Cs: %f\n", end - start);

    start = current_time();
    for (int j = 0; j < num_ots; ++j) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));
        // choose random k
        mpz_urandomb(ks[j], s->p.rnd, FIELD_SIZE * 8);
        mpz_mod(ks[j], ks[j], s->p.q);
        // compute pks = g^k
        mpz_powm(pks, s->p.g, ks[j], s->p.p);
        (void) mpz_invert(pk0, pks, s->p.p);
        mpz_mul(pk0, pk0, Cs[0]);
        mpz_mod(pk0, pk0, s->p.p);
        mpz_set(pk0, choice == 0 ? pks : pk0);
        mpz_to_array(buf, pk0, sizeof buf);

        // send pk0 to sender
        if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
    }
    end = current_time();
    fprintf(stderr, "SEND: pk0s: %f\n", end - start);

    for (int j = 0; j < num_ots; ++j) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        // compute (g^r)^k = pks^r
        mpz_powm(ks[j], gr, ks[j], s->p.p);

        start = current_time();
        for (int i = 0; i < N; ++i) {
            // get H xor M0 from sender
            if (pyrecv(s->sockfd, msg, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }

            mpz_to_array(buf, ks[j], sizeof buf);
            build_hash(from, buf, i, maxlength);
            xorarray(msg, maxlength, from, maxlength);
            if (i == choice) {
                PyObject *str = PyString_FromStringAndSize(msg, maxlength);
                if (str == NULL) {
                    err = 1;
                    goto cleanup;
                } else {
                    PyTuple_SetItem(py_return, j, str);
                }
            }
        }
        end = current_time();
        fprintf(stderr, "RECEIVE: hashes: %f\n", end - start);
    }

 cleanup:
    mpz_clears(gr, pk0, pks, NULL);

    if (ks) {
        for (int j = 0; j < num_ots; ++j) {
            mpz_clear(ks[j]);
        }
        free(ks);
    }
    if (Cs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Cs[i]);
        free(Cs);
    }
    if (msg)
        free(msg);
    if (from)
        free(from);

    // TODO: clean up py_return if error

    if (err)
        return NULL;
    else
        return py_return;
}
