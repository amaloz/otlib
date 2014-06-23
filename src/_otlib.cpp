#include <Python.h>

#include "naorpinkas.h"
#include "otextension.h"
#include "pvw.h"
#include "state.h"

static PyMethodDef
methods[] = {
    {"init", state_init, METH_VARARGS, "initialize OT state."},
    {"np_send", np_send, METH_VARARGS, "sender operation for Naor-Pinkas OT."},
    {"np_receive", np_receive, METH_VARARGS, "receiver operation for Naor-Pinkas OT."},
    {"otext_send", otext_send, METH_VARARGS, "TODO."}, // TODO:
    {"otext_matrix_xor", otext_matrix_xor, METH_VARARGS, "TODO."}, // TODO:
    {"otext_receive", otext_receive, METH_VARARGS, "TODO."}, // TODO:
    {"pvw_send", pvw_send, METH_VARARGS, "sender operation for PVW OT."},
    {"pvw_receive", pvw_receive, METH_VARARGS, "receiver operation for PVW OT."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_otlib(void)
{
    (void) Py_InitModule("_otlib", methods);
}
