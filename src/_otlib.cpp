#include <Python.h>

#include "otext_iknp.h"
#include "otext_nnob.h"
#include "ot_np.h"
#include "ot_pvw.h"
#include "state.h"

static PyMethodDef
methods[] = {
    {"init", state_init, METH_VARARGS, "initialize OT state."},
    {"ot_np_send", ot_np_send, METH_VARARGS, "sender operation for Naor-Pinkas OT."},
    {"ot_np_receive", ot_np_receive, METH_VARARGS, "receiver operation for Naor-Pinkas OT."},
    {"ot_pvw_send", ot_pvw_send, METH_VARARGS, "sender operation for PVW OT."},
    {"ot_pvw_receive", ot_pvw_receive, METH_VARARGS, "receiver operation for PVW OT."},
    {"otext_iknp_send", otext_iknp_send, METH_VARARGS, "TODO."}, // TODO:
    {"otext_iknp_matrix_xor", otext_iknp_matrix_xor, METH_VARARGS, "TODO."}, // TODO:
    {"otext_iknp_receive", otext_iknp_receive, METH_VARARGS, "TODO."}, // TODO:
    // {"otext_nnob_send", otext_nnob_send, METH_VARARGS, "TODO."}, // TODO:
    // {"otext_nnob_receive", otext_nnob_receive, METH_VARARGS, "TODO."}, // TODO:
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_otlib(void)
{
    (void) Py_InitModule("_otlib", methods);
}
