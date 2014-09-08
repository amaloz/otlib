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

