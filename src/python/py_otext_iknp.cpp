#include "py_otext_iknp.h"
#include "py_ot.h"

#include "../otext_iknp.h"
#include "../crypto.h"
#include "../utils.h"

static unsigned char *
to_array(PyObject *columns, int nrows, int ncols)
{
    unsigned char *array;
    double start, end;

    start = current_time();
    array = (unsigned char *) malloc(nrows * ncols / 8);
    if (array == NULL)
        return NULL;
    memset(array, '\0', nrows * ncols / 8);

    for (int i = 0; i < ncols; ++i) {
        char *col;
        Py_ssize_t collen;

        (void) PyBytes_AsStringAndSize(PySequence_GetItem(columns, i),
                                       &col, &collen);
        assert(collen * 8 == nrows);
        memcpy(array + i * collen, col, collen);
    }
    end = current_time();
    fprintf(stderr, "to_array: %f\n", end - start);

    return array;
}

static const unsigned char MASK_BIT[8] =
    {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
static const unsigned char CMASK_BIT[8] =
    {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};
static const unsigned char MASK_SET_BIT_C[2][8] =
    {{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1},
     {   0,    0,    0,    0,   0,   0,   0,   0}};

static inline unsigned char
get_bit(const unsigned char *array, int idx)
{
    return !!(array[idx >> 3] & MASK_BIT[idx & 0x7]);
}

static inline void
set_bit(unsigned char *array, int idx, unsigned char bit)
{
    array[idx >> 3] = (array[idx >> 3] & CMASK_BIT[idx & 0x7]) | MASK_SET_BIT_C[!bit][idx & 0x7];
}

static unsigned char *
transpose(unsigned char *array, int nrows, int ncols)
{
    unsigned char *tarray;
    double start, end;

    start = current_time();
    tarray = (unsigned char *) malloc(nrows * ncols / 8);
    if (tarray == NULL)
        return NULL;
    memset(tarray, '\0', nrows * ncols / 8);

    for (int i = 0; i < ncols; ++i) {
        for (int j = 0; j < nrows; ++j) {
            unsigned char bit = get_bit(array, i * nrows + j);
            set_bit(tarray, j * ncols + i, bit);
        }
    }
    end = current_time();
    fprintf(stderr, "transpose: %f\n", end - start);

    return tarray;
}

PyObject *
otext_iknp_matrix_xor(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_matrix, *py_return = NULL;
    struct state *st;
    char *r, *xors;
    Py_ssize_t rlen;
    unsigned int nchoices, secparam;

    if (!PyArg_ParseTuple(args, "OOs#II", &py_state, &py_matrix, &r, &rlen,
                          &nchoices, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    assert(nchoices % 8 == 0);
    nchoices /= 8;

    assert((int) rlen == (int) nchoices);

    xors = (char *) malloc(sizeof(char) * nchoices);
    if (xors == NULL)
        return NULL;

    py_return = PyTuple_New(secparam);

    for (unsigned int i = 0; i < secparam; ++i) {
        PyObject *tuple;
        char *t;
        Py_ssize_t tlen;

        (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_matrix, i), &t, &tlen);
        assert(tlen == nchoices);
        memcpy(xors, t, nchoices);
        xorarray((unsigned char *) xors, nchoices, (unsigned char *) r, nchoices);

        tuple = PyTuple_New(2);
        PyTuple_SetItem(tuple, 0, PySequence_GetItem(py_matrix, i));
        PyTuple_SetItem(tuple, 1, PyString_FromStringAndSize(xors, nchoices));

        PyTuple_SetItem(py_return, i, tuple);
    }

    return py_return;
}

PyObject *
py_otext_iknp_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs, *py_qt;
    struct state *st;
    long m, err = 0;
    char *s;
    unsigned char *array = NULL, *tarray = NULL;
    int slen;
    unsigned int msglength, secparam;

    if (!PyArg_ParseTuple(args, "OOOs#II", &py_state, &py_msgs,
                          &py_qt, &s, &slen, &msglength, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_msgs)) == -1)
        return NULL;

    array = to_array(py_qt, m, secparam);
    if (array == NULL) {
        err = 1;
        goto cleanup;
    }
    tarray = transpose(array, m, secparam);
    if (tarray == NULL) {
        err = 1;
        goto cleanup;
    }

    err = otext_iknp_send(st, py_msgs, m, msglength, secparam / 8, s, slen,
                          tarray, py_ot_msg_reader, py_ot_item_reader);

 cleanup:
    if (array)
        free(array);
    if (tarray)
        free(tarray);

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
py_otext_iknp_recv(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_T, *py_choices, *py_return = NULL;
    struct state *st;
    unsigned char *array = NULL, *tarray = NULL;
    int nchoices, err = 0;
    unsigned int maxlength, secparam;

    if (!PyArg_ParseTuple(args, "OOOII", &py_state, &py_choices, &py_T,
                          &maxlength, &secparam))
        return NULL;

    // fprintf(stderr, "RUNNING\n");

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((nchoices = PySequence_Length(py_choices)) == -1) {
        err = 1;
        goto cleanup;
    }

    array = to_array(py_T, nchoices, secparam);
    if (array == NULL) {
        err = 1;
        goto cleanup;
    }
    tarray = transpose(array, nchoices, secparam);
    if (tarray == NULL) {
        err = 1;
        goto cleanup;
    }

    py_return = PyTuple_New(nchoices);

    // fprintf(stderr, "ENTERING\n");

    err = otext_iknp_recv(st, py_choices, nchoices, maxlength, secparam, tarray,
                          py_return, py_ot_choice_reader, py_ot_msg_writer);

 cleanup:
    if (array)
        free(array);
    if (tarray)
        free(tarray);

    if (err)
        return NULL;
    else
        return py_return;
}
