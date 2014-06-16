#ifndef __OTLIB_STATE_H__
#define __OTLIB_STATE_H__

#include <Python.h>
#include <gmp.h>

#include "gmputils.h"

struct state {
    struct params p;
    long length;
    int sockfd;
    int serverfd;
};

PyObject *
state_init(PyObject *self, PyObject *args);

#endif
