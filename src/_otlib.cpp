#include <Python.h>

#include "naorpinkas.h"
#include "state.h"

static PyMethodDef
methods[] = {
    {"init", state_init, METH_VARARGS, "initialize OT state."},
    {"np_send", np_send, METH_VARARGS, "sender operation for Naor-Pinkas OT."},
    {"np_receive", np_receive, METH_VARARGS, "receiver operation for Naor-Pinkas OT."},
    // {"otext_send", otext_send, METH_VARARGS, "TODO."}, // TODO:
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_otlib(void)
{
    (void) Py_InitModule("_otlib", methods);
}
