#ifndef __OTLIB_OTEXT_IKNP_H__
#define __OTLIB_OTEXT_IKNP_H__

#include <Python.h>

PyObject *
otext_iknp_send(PyObject *self, PyObject *args);

PyObject *
otext_iknp_matrix_xor(PyObject *self, PyObject *args);

PyObject *
otext_iknp_receive(PyObject *self, PyObject *args);

#endif
