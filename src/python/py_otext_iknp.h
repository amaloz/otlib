#ifndef __OTLIB_PY_OTEXT_IKNP_H__
#define __OTLIB_PY_OTEXT_IKNP_H__

#include <Python.h>

PyObject *
py_otext_iknp_send(PyObject *self, PyObject *args);

PyObject *
otext_iknp_matrix_xor(PyObject *self, PyObject *args);

PyObject *
py_otext_iknp_recv(PyObject *self, PyObject *args);

#endif
