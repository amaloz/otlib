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
    char *s, *msg = NULL;
    int slen;
    /* max length of messages in bytes */
    unsigned int maxlength;

    if (!PyArg_ParseTuple(args, "OOIs#O", &py_state, &py_msgs, &maxlength,
                          &s, &slen, &py_qt))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    msg = (char *) pymalloc(sizeof(char) * maxlength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }

    if ((m = PySequence_Length(py_msgs)) == -1)
        return NULL;

    for (int j = 0; j < m; ++j) {
        PyObject *py_input;

        py_input = PySequence_GetItem(py_msgs, j);
        for (int i = 0; i < 2; ++i) {
            char *m, *q;
            Py_ssize_t mlen, qlen;
            char hash[SHA_DIGEST_LENGTH];
            unsigned int length = 0;

            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_qt, j),
                                           &q, &qlen);
            assert(qlen <= (int) sizeof hash);
            assert(slen <= (int) sizeof hash);
            (void) memset(hash, '\0', sizeof hash);
            xorarray(hash, sizeof hash, q, qlen);
            if (i == 1) {
                xorarray(hash, sizeof hash, s, slen);
            }

            while (length < maxlength) {
                SHA_CTX c;
                int n;

                (void) SHA1_Init(&c);
                if (length == 0) {
                    (void) SHA1_Update(&c, &j, sizeof j);
                    (void) SHA1_Update(&c, hash, sizeof hash);
                    (void) SHA1_Final((unsigned char *) hash, &c);
                } else {
                    (void) SHA1_Update(&c, msg + length - sizeof hash, sizeof hash);
                    (void) SHA1_Final((unsigned char *) hash, &c);
                }
                n = MIN(maxlength - length, (int) sizeof hash);
                (void) memcpy(msg + length, hash, n);
                length += n;
            }

            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            assert(mlen <= maxlength);
            xorarray(msg, maxlength, m, mlen);
            if (pysend(st->sockfd, msg, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }

        }
    }

 cleanup:
    if (msg)
        free(msg);

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
    char *from = NULL, *msg = NULL;
    int m, err = 0;
    unsigned int maxlength;

    if (!PyArg_ParseTuple(args, "OOIO", &py_state, &py_choices, &maxlength,
                          &py_T))
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

    py_return = PyTuple_New(m);

    for (int j = 0; j < m; ++j) {
        int choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        for (int i = 0; i < 2; ++i) {
            char hash[SHA_DIGEST_LENGTH];
            char *t;
            Py_ssize_t tlen;
            unsigned int length = 0;

            if (pyrecv(st->sockfd, from, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }

            while (length < maxlength) {
                SHA_CTX c;
                int n;

                (void) SHA1_Init(&c);
                if (length == 0) {
                    (void) SHA1_Update(&c, &j, sizeof j);
                    (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_T, j),
                                                   &t, &tlen);
                    assert(tlen <= (int) sizeof hash);
                    (void) memset(hash, '\0', sizeof hash);
                    (void) memcpy(hash, t, tlen);
                    (void) SHA1_Update(&c, hash, sizeof hash);
                    (void) SHA1_Final((unsigned char *) hash, &c);
                } else {
                    (void) SHA1_Update(&c, msg + length - sizeof hash, sizeof hash);
                    (void) SHA1_Final((unsigned char *) hash, &c);
                }
                n = MIN(maxlength - length, (int) sizeof hash);
                (void) memcpy(msg + length, hash, n);
                length += n;
            }

            if (i == choice) {
                xorarray(from, maxlength, msg, maxlength);
                PyObject *str = PyString_FromStringAndSize(from, maxlength);
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
    if (from)
        free(from);
    if (msg)
        free(msg);

    if (err)
        return NULL;
    else
        return py_return;
}

