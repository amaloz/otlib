#ifndef __OTLIB_OTEXTENSION_H__
#define __OTLIB_OTEXTENSION_H__

#include <Python.h>

PyObject *
otext_send(PyObject *self, PyObject *args);

PyObject *
otext_receive(PyObject *self, PyObject *args);

#endif
