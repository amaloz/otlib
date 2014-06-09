#ifndef __OTLIB_NAORPINKAS_H__
#define __OTLIB_NAORPINKAS_H__

#include <Python.h>

PyObject *
np_send(PyObject *self, PyObject *args);

PyObject *
np_receive(PyObject *self, PyObject *args);

#endif
