/*
 * Implementation of semi-honest OT as detailed by Naor and Pinkas [1].
 *
 * Author: Alex J. Malozemoff <amaloz@cs.umd.edu>
 *
 * [1] "Efficient Oblivious Transfer Protocols."
 *     N. Naor, B. Pinkas. SODA 2001.
 */
#include "ot_np.h"

#include "crypto.h"
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

// static const char *tag = "OT-NP";

#define ERROR { err = 1; goto cleanup; }

PyObject *
ot_np_send(PyObject *self, PyObject *args)
{
    mpz_t r, gr, pk, pk0;
    mpz_t *Cs = NULL, *Crs = NULL, *pk0s = NULL;
    char buf[FIELD_SIZE], *msg = NULL;
    PyObject *py_state, *py_msgs;
    long N, num_ots, err = 0;
    int msglength;
    struct state *st;

    if (!PyArg_ParseTuple(args, "OOi", &py_state, &py_msgs, &msglength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_msgs)) == -1)
        return NULL;

    if ((N = PySequence_Length(PySequence_GetItem(py_msgs, 0))) == -1)
        return NULL;

    /* check to make sure all OTs are 1-out-of-N OTs and all messages are of
       length <= msglength */
    for (int j = 0; j < num_ots; ++j) {
        PyObject *tuple = PySequence_GetItem(py_msgs, j);
        if (PySequence_Length(tuple) != N) {
            PyErr_SetString(PyExc_TypeError, "unmatched input length");
            return NULL;
        }
        for (int i = 0; i < N; ++i) {
            if (PyString_Size(PySequence_GetItem(tuple, i)) > msglength) {
                PyErr_SetString(PyExc_TypeError, "message > msglength");
                return NULL;
            }
        }
    }

    mpz_inits(r, gr, pk, pk0, NULL);

    msg = (char *) pymalloc(sizeof(char) * msglength);
    if (msg == NULL)
        ERROR;
    Cs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL)
        ERROR;
    Crs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Crs == NULL)
        ERROR;
    pk0s = (mpz_t *) pymalloc(sizeof(mpz_t) * num_ots);
    if (pk0s == NULL)
        ERROR;

    for (int i = 0; i < num_ots; ++i) {
        mpz_init(pk0s[i]);
    }

    // choose r \in_R Zq
    random_element(r, &st->p);
    // compute g^r
    mpz_powm(gr, st->p.g, r, st->p.p);

    // choose C_i's \in_R Zq
    for (int i = 0; i < N - 1; ++i) {
        mpz_inits(Cs[i], Crs[i], NULL);
        random_element(Cs[i], &st->p);
    }

    // send g^r to receiver
    mpz_to_array(buf, gr, sizeof buf);
    if (pysend(st->sockfd, buf, sizeof buf, 0) == -1)
        ERROR;
    // send Cs to receiver
    for (int i = 0; i < N - 1; ++i) {
        mpz_to_array(buf, Cs[i], sizeof buf);
        if (pysend(st->sockfd, buf, sizeof buf, 0) == -1)
            ERROR;
    }

    for (int i = 0; i < N - 1; ++i) {
        // compute C_i^r
        mpz_powm(Crs[i], Cs[i], r, st->p.p);
    }

    for (int j = 0; j < num_ots; ++j) {
        // get pk0 from receiver
        if (pyrecv(st->sockfd, buf, sizeof buf, 0) == -1)
            ERROR;
        array_to_mpz(pk0s[j], buf, sizeof buf);
    }

    for (int j = 0; j < num_ots; ++j) {
        PyObject *py_input = PySequence_GetItem(py_msgs, j);
        for (int i = 0; i < N; ++i) {
            Py_ssize_t mlen;
            char *m;

            if (i == 0) {
                // compute pk0^r
                mpz_powm(pk0, pk0s[j], r, st->p.p);
                mpz_to_array(buf, pk0, sizeof buf);
                (void) mpz_invert(pk0, pk0, st->p.p);
            } else {
                mpz_mul(pk, pk0, Crs[i - 1]);
                mpz_mod(pk, pk, st->p.p);
                mpz_to_array(buf, pk, sizeof buf);
            }
            sha1_hash(msg, msglength, i, (unsigned char *) buf, FIELD_SIZE);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            assert(mlen <= msglength);
            xorarray((unsigned char *) msg, msglength,
                     (unsigned char *) m, mlen);
            if (pysend(st->sockfd, msg, msglength, 0) == -1)
                ERROR;
        }
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
ot_np_receive(PyObject *self, PyObject *args)
{
    mpz_t gr, pk0, pks;
    mpz_t *Cs = NULL, *ks = NULL;
    char buf[FIELD_SIZE], *from = NULL, *msg = NULL;
    struct state *s;
    PyObject *py_state, *py_choices, *py_return = NULL;
    int num_ots, err = 0;
    int N, msglength;

    if (!PyArg_ParseTuple(args, "OOii", &py_state, &py_choices, &N, &msglength))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_choices)) == -1)
        return NULL;

    mpz_inits(gr, pk0, pks, NULL);

    py_return = PyTuple_New(num_ots);

    msg = (char *) pymalloc(sizeof(char) * msglength);
    if (msg == NULL)
        ERROR;
    from = (char *) pymalloc(sizeof(char) * msglength);
    if (from == NULL)
        ERROR;
    Cs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL)
        ERROR;
    for (int i = 0; i < N - 1; ++i) {
        mpz_init(Cs[i]);
    }
    ks = (mpz_t *) pymalloc(sizeof(mpz_t) * num_ots);
    if (ks == NULL)
        ERROR;
    for (int j = 0; j < num_ots; ++j) {
        mpz_init(ks[j]);
    }

    // get g^r from sender
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1)
        ERROR;
    array_to_mpz(gr, buf, sizeof buf);

    // get Cs from sender
    for (int i = 0; i < N - 1; ++i) {
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1)
            ERROR;
        array_to_mpz(Cs[i], buf, sizeof buf);
    }

    for (int j = 0; j < num_ots; ++j) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));
        // choose random k
        mpz_urandomb(ks[j], s->p.rnd, FIELD_SIZE * 8);
        mpz_mod(ks[j], ks[j], s->p.q);
        // compute pks = g^k
        mpz_powm(pks, s->p.g, ks[j], s->p.p);
        // compute pk0 = C_1 / g^k regardless of whether our choice is 0 or 1 to
        // avoid a potential side-channel attack
        (void) mpz_invert(pk0, pks, s->p.p);
        mpz_mul(pk0, pk0, Cs[0]);
        mpz_mod(pk0, pk0, s->p.p);
        mpz_set(pk0, choice == 0 ? pks : pk0);
        mpz_to_array(buf, pk0, sizeof buf);
        // send pk0 to sender
        if (pysend(s->sockfd, buf, sizeof buf, 0) == -1)
            ERROR;
    }

    for (int j = 0; j < num_ots; ++j) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        // compute decryption key (g^r)^k
        mpz_powm(ks[j], gr, ks[j], s->p.p);

        for (int i = 0; i < N; ++i) {
            // get H xor M0 from sender
            if (pyrecv(s->sockfd, msg, msglength, 0) == -1)
                ERROR;
            mpz_to_array(buf, ks[j], sizeof buf);
            sha1_hash(from, msglength, i, (unsigned char *) buf, FIELD_SIZE);
            xorarray((unsigned char *) msg, msglength,
                     (unsigned char *) from, msglength);
            if (i == choice) {
                PyObject *str = PyString_FromStringAndSize(msg, msglength);
                if (str == NULL) {
                    ERROR;
                } else
                    PyTuple_SetItem(py_return, j, str);
            }
        }
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
