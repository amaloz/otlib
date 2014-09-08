#include "py_ot_np.h"
#include "py_ot.h"

#include "../ot_np.h"
#include "../crypto.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define ERROR { err = 1; goto cleanup; }

PyObject *
py_ot_np_send(PyObject *self, PyObject *args)
{
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

    err = ot_np_send(st, py_msgs, msglength, num_ots, N, py_ot_msg_reader,
                     py_ot_item_reader);

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
py_ot_np_recv(PyObject *self, PyObject *args)
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

    msg = (char *) malloc(sizeof(char) * msglength);
    if (msg == NULL)
        ERROR;
    from = (char *) malloc(sizeof(char) * msglength);
    if (from == NULL)
        ERROR;
    Cs = (mpz_t *) malloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL)
        ERROR;
    for (int i = 0; i < N - 1; ++i) {
        mpz_init(Cs[i]);
    }
    ks = (mpz_t *) malloc(sizeof(mpz_t) * num_ots);
    if (ks == NULL)
        ERROR;
    for (int j = 0; j < num_ots; ++j) {
        mpz_init(ks[j]);
    }

    // get g^r from sender
    if (recv(s->sockfd, buf, sizeof buf, 0) == -1)
        ERROR;
    array_to_mpz(gr, buf, sizeof buf);

    // get Cs from sender
    for (int i = 0; i < N - 1; ++i) {
        if (recv(s->sockfd, buf, sizeof buf, 0) == -1)
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
        if (send(s->sockfd, buf, sizeof buf, 0) == -1)
            ERROR;
    }

    for (int j = 0; j < num_ots; ++j) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        // compute decryption key (g^r)^k
        mpz_powm(ks[j], gr, ks[j], s->p.p);

        for (int i = 0; i < N; ++i) {
            // get H xor M0 from sender
            if (recv(s->sockfd, msg, msglength, 0) == -1)
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
