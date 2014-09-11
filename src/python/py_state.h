#ifndef __OTLIB_PY_STATE_H__
#define __OTLIB_PY_STATE_H__

#include <Python.h>

PyObject *
py_state_init(PyObject *self, PyObject *args);

PyObject *
py_state_cleanup(PyObject *self, PyObject *args);

#endif
