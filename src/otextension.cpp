#include <Python.h>

#include "state.h"

#include <gmp.h>

#define SECPARAM 80

PyObject *
otext_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs;
    struct state *st;
    int k, m;

    if (!PyArg_ParseTuple(args, "OOii", &py_state, &py_msgs, &k, &m))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if (PySequence_Length(py_msgs) != m) {
        PyErr_SetString(PyExc_TypeError, "number of messages does not match 'm' given");
        return NULL;
    }

    // check to make sure all inputs to the OTs are of the same length
    for (int i = 0; i < m; ++i) {
        if (PyByteArray_Size(PySequence_GetItem(py_msgs, i)) != k / 8) {
            PyErr_SetString(PyExc_TypeError, "message length does not match 'k' given");
            return NULL;
        }
    }

    Py_RETURN_NONE;
}
