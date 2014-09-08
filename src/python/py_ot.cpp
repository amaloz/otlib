#include <Python.h>

#include "py_ot.h"

void *
py_ot_msg_reader(void *msgs, int idx)
{
    return (void *) PySequence_GetItem((PyObject *) msgs, idx);
}

void
py_ot_item_reader(void *item, int idx, void *m, ssize_t *mlen)
{
    (void) PyBytes_AsStringAndSize(PySequence_GetItem((PyObject *) item, idx),
                                   (char **) m, mlen);
}

int
py_ot_choice_reader(void *choices, int idx)
{
    return PyLong_AsLong(PySequence_GetItem((PyObject *) choices, idx));
}

int
py_ot_msg_writer(void *out, int idx, void *msg, size_t maxlength)
{
    PyObject *str = PyString_FromStringAndSize((char *) msg, maxlength);
    if (str == NULL)
        return 1;
    PyTuple_SetItem((PyObject *) out, idx, str);
    return 0;
}
