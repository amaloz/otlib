#ifndef __OTLIB_PVW_H__
#define __OTLIB_PVW_H__

#include <Python.h>

PyObject *
pvw_send(PyObject *self, PyObject *args);

PyObject *
pvw_receive(PyObject *self, PyObject *args);

#endif
