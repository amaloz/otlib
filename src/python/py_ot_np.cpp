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
    PyObject *state, *choices, *out = NULL;
    struct state *st;
    int nchoices, err = 0;
    int N, maxlength;

    if (!PyArg_ParseTuple(args, "OOii", &state, &choices, &N, &maxlength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(state, NULL);
    if (st == NULL)
        return NULL;

    if ((nchoices = PySequence_Length(choices)) == -1)
        return NULL;

    out = PyTuple_New(nchoices);
    
    err = ot_np_recv(st, choices, nchoices, maxlength, N, out,
                     py_ot_choice_reader, py_ot_msg_writer);

    // TODO: cleanup out on error

    if (err)
        return NULL;
    else
        return out;
}
