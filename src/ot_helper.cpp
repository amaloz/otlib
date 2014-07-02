#include "ot_helper.h"

#include <gmp.h>

#include "state.h"

PyObject *
ot_helper_random_seed(PyObject *self, PyObject *args)
{
    PyObject *py_state;
    unsigned int length;
    char *str;
    mpz_t r;
    struct state *st;

    if (!PyArg_ParseTuple(args, "OI", &py_state, &length))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    mpz_init(r);
    mpz_urandomb(r, st->p.rnd, length);
    str = mpz_get_str(NULL, 2, r);
    mpz_clear(r);

    // TODO: convert to python
}
