#include "otextension.h"

#include "state.h"
#include "utils.h"

#include <gmp.h>
#include <openssl/sha.h>

#define SECPARAM 80

PyObject *
otext_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs, *py_qt;
    struct state *st;
    long m, err = 0;
    char *s;
    int slen;

    if (!PyArg_ParseTuple(args, "OOs#O", &py_state, &py_msgs, &s, &slen, &py_qt))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_msgs)) == -1)
        return NULL;

    for (int j = 0; j < m; ++j) {
        PyObject *py_input;

        py_input = PySequence_GetItem(py_msgs, j);
        for (int i = 0; i < 2; ++i) {
            SHA_CTX c;
            char *m, *q;
            Py_ssize_t mlen, qlen;
            char hash[SHA_DIGEST_LENGTH];

            (void) memset(hash, '\0', sizeof hash);
            (void) SHA1_Init(&c);
            (void) SHA1_Update(&c, &j, sizeof j);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_qt, j),
                                           &q, &qlen);
            // xorarray(hash, sizeof hash, q, qlen);
            // if (i == 1) {
            //     xorarray(hash, sizeof hash, s, slen);
            // }
            // (void) SHA1_Update(&c, hash, sizeof hash);
            (void) SHA1_Update(&c, q, qlen);
            // fprintf(stderr, "%ld\n", qlen);
            (void) SHA1_Final((unsigned char *) hash, &c);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            xorarray(hash, sizeof hash, m, mlen);
            if (pysend(st->sockfd, hash, sizeof hash, 0) == -1) {
                err = 1;
                goto cleanup;
            }

        }
    }

 cleanup:
    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
otext_receive(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_T, *py_choices, *py_return = NULL;
    struct state *st;
    int m, err = 0;

    if (!PyArg_ParseTuple(args, "OOO", &py_state, &py_choices, &py_T))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_choices)) == -1)
        return NULL;

    py_return = PyTuple_New(m);

    for (int j = 0; j < m; ++j) {
        int choice;
        char *t;
        Py_ssize_t tlen;
        char from[SHA_DIGEST_LENGTH], hash[SHA_DIGEST_LENGTH];

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        for (int i = 0; i < 2; ++i) {
            SHA_CTX c;

            if (pyrecv(st->sockfd, from, sizeof from, 0) == -1) {
                err = 1;
                goto cleanup;
            }

            (void) SHA1_Init(&c);
            (void) SHA1_Update(&c, &j, sizeof j);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_T, j),
                                           &t, &tlen);
            // fprintf(stderr, "%ld\n", tlen);
            (void) SHA1_Update(&c, t, tlen);
            (void) SHA1_Final((unsigned char *) hash, &c);
            if (i == choice) {
                xorarray(from, sizeof from, hash, sizeof hash);
                PyObject *str = PyString_FromString(from);
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
    if (err)
        return NULL;
    else
        return py_return;
}

