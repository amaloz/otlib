/*
 * Implementation if semi-honest OT extension as detailed by Ishai et al. [1].
 *
 * Author: Alex J. Malozemoff <amaloz@cs.umd.edu>
 *
 * [1] "Extending Oblivious Transfer Efficiently."
 *     Y. Ishai, J. Kilian, K. Nissim, E. Petrank. CRYPTO 2003.
 */
#include "otext_iknp.h"

#include "crypto.h"
#include "net.h"
#include "state.h"
#include "utils.h"

#include <gmp.h>
#include <openssl/sha.h>

static const unsigned char MASK_BIT[8] =
    {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
static const unsigned char CMASK_BIT[8] =
    {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};
static const unsigned char MASK_SET_BIT_C[2][8] =
    {{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1},
     {   0,    0,    0,    0,   0,   0,   0,   0}};

static inline unsigned char
get_bit(const unsigned char *array, int idx)
{
    return !!(array[idx >> 3] & MASK_BIT[idx & 0x7]);
}

static inline void
set_bit(unsigned char *array, int idx, unsigned char bit)
{
    array[idx >> 3] = (array[idx >> 3] & CMASK_BIT[idx & 0x7]) | MASK_SET_BIT_C[!bit][idx & 0x7];
}

static unsigned char *
to_array(PyObject *columns, int nrows, int ncols)
{
    unsigned char *array;
    double start, end;

    start = current_time();
    array = (unsigned char *) pymalloc(nrows * ncols / 8);
    if (array == NULL)
        return NULL;
    memset(array, '\0', nrows * ncols / 8);

    for (int i = 0; i < ncols; ++i) {
        char *col;
        Py_ssize_t collen;

        (void) PyBytes_AsStringAndSize(PySequence_GetItem(columns, i),
                                       &col, &collen);
        assert(collen * 8 == nrows);
        memcpy(array + i * collen, col, collen);
    }
    end = current_time();
    fprintf(stderr, "to_array: %f\n", end - start);

    return array;
}

static unsigned char *
transpose(unsigned char *array, int nrows, int ncols)
{
    unsigned char *tarray;
    double start, end;

    start = current_time();
    tarray = (unsigned char *) pymalloc(nrows * ncols / 8);
    if (tarray == NULL)
        return NULL;
    memset(tarray, '\0', nrows * ncols / 8);

    for (int i = 0; i < ncols; ++i) {
        for (int j = 0; j < nrows; ++j) {
            unsigned char bit = get_bit(array, i * nrows + j);
            set_bit(tarray, j * ncols + i, bit);
        }
    }
    end = current_time();
    fprintf(stderr, "transpose: %f\n", end - start);

    return tarray;
}

PyObject *
otext_iknp_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs, *py_qt;
    struct state *st;
    long m, err = 0;
    char *s, *msg = NULL;
    unsigned char *array = NULL, *tarray = NULL;
    int slen;
    unsigned int msglength, secparam;
    double start, end;

    if (!PyArg_ParseTuple(args, "OOOs#II", &py_state, &py_msgs,
                          &py_qt, &s, &slen, &msglength, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_msgs)) == -1)
        return NULL;

    msg = (char *) pymalloc(sizeof(char) * msglength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }

    array = to_array(py_qt, m, secparam);
    if (array == NULL) {
        err = 1;
        goto cleanup;
    }
    tarray = transpose(array, m, secparam);
    if (tarray == NULL) {
        err = 1;
        goto cleanup;
    }

    start = current_time();
    for (int j = 0; j < m; ++j) {
        PyObject *py_input;

        py_input = PySequence_GetItem(py_msgs, j);
        for (int i = 0; i < 2; ++i) {
            char *m;
            unsigned char *q;
            Py_ssize_t mlen;
            char hash[SHA_DIGEST_LENGTH];

            q = &tarray[j * (secparam / 8)];
            assert(slen <= (int) sizeof hash);
            (void) memset(hash, '\0', sizeof hash);
            xorarray((unsigned char *) hash, sizeof hash, q, secparam / 8);
            if (i == 1) {
                xorarray((unsigned char *) hash, sizeof hash,
                         (unsigned char *) s, slen);
            }
            sha1_hash(msg, msglength, j,
                      (unsigned char *) hash, SHA_DIGEST_LENGTH);

            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            assert(mlen <= msglength);
            xorarray((unsigned char *) msg, msglength,
                     (unsigned char *) m, mlen);
            if (pysend(st->sockfd, msg, msglength, 0) == -1) {
                err = 1;
                goto cleanup;
            }
        }
    }
    end = current_time();
    fprintf(stderr, "hash and send: %f\n", end - start);

 cleanup:
    if (msg)
        free(msg);
    if (array)
        free(array);
    if (tarray)
        free(tarray);

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
otext_iknp_matrix_xor(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_T, *py_return = NULL, *py_t_xor_r;
    struct state *st;
    char *r, *t_xor_r;
    Py_ssize_t rlen;
    unsigned int m, secparam;

    if (!PyArg_ParseTuple(args, "OOs#II", &py_state, &py_T, &r, &rlen,
                          &m, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    assert(m % 8 == 0);
    m = m / 8;

    assert((int) rlen == (int) m);

    t_xor_r = (char *) pymalloc(sizeof(char) * m);
    if (t_xor_r == NULL)
        return NULL;

    py_return = PyTuple_New(secparam);

    for (unsigned int i = 0; i < secparam; ++i) {
        PyObject *tuple;
        char *t;
        Py_ssize_t tlen;

        (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_T, i), &t, &tlen);
        assert(tlen == m);
        memcpy(t_xor_r, t, m);
        xorarray((unsigned char *) t_xor_r, m, (unsigned char *) r, m);
        py_t_xor_r = PyString_FromStringAndSize(t_xor_r, m);

        tuple = PyTuple_New(2);
        PyTuple_SetItem(tuple, 0, PySequence_GetItem(py_T, i));
        PyTuple_SetItem(tuple, 1, py_t_xor_r);

        PyTuple_SetItem(py_return, i, tuple);
    }

    return py_return;
}

PyObject *
otext_iknp_receive(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_T, *py_choices, *py_return = NULL;
    struct state *st;
    char *from = NULL, *msg = NULL;
    unsigned char *array = NULL, *tarray = NULL;
    int m, err = 0;
    unsigned int maxlength, secparam;
    double start, end;

    if (!PyArg_ParseTuple(args, "OOOII", &py_state, &py_choices, &py_T, &maxlength,
                          &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    from = (char *) pymalloc(sizeof(char) * maxlength);
    if (from == NULL) {
        err = 1;
        goto cleanup;
    }
    msg = (char *) pymalloc(sizeof(char) * maxlength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }
    
    if ((m = PySequence_Length(py_choices)) == -1)
        return NULL;

    array = to_array(py_T, m, secparam);
    if (array == NULL) {
        err = 1;
        goto cleanup;
    }
    tarray = transpose(array, m, secparam);
    if (tarray == NULL) {
        err = 1;
        goto cleanup;
    }

    py_return = PyTuple_New(m);

    start = current_time();
    for (int j = 0; j < m; ++j) {
        int choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        for (int i = 0; i < 2; ++i) {
            char hash[SHA_DIGEST_LENGTH];
            unsigned char *t;
            PyObject *str;

            if (pyrecv(st->sockfd, from, maxlength, 0) == -1) {
                fprintf(stderr, "%d\n", j);
                err = 1;
                goto cleanup;
            }

            t = &tarray[j * (secparam / 8)];
            (void) memset(hash, '\0', sizeof hash);
            (void) memcpy(hash, t, secparam / 8);
            sha1_hash(msg, maxlength, j, (unsigned char *) hash, SHA_DIGEST_LENGTH);

            xorarray((unsigned char *) from, maxlength,
                     (unsigned char *) msg, maxlength);
            str = PyString_FromStringAndSize(from, maxlength);
            if (str == NULL) {
                err = 1;
                goto cleanup;
            }
            if (i == choice) {
                PyTuple_SetItem(py_return, j, str);
            }
        }
    }
    end = current_time();
    fprintf(stderr, "hash and receive: %f\n", end - start);

 cleanup:
    if (from)
        free(from);
    if (msg)
        free(msg);
    if (array)
        free(array);
    if (tarray)
        free(tarray);

    if (err)
        return NULL;
    else
        return py_return;
}
