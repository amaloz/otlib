#include "py_otext_iknp.h"

#include "../otext_iknp.h"
#include "../crypto.h"
#include "../utils.h"

static unsigned char *
to_array(PyObject *columns, int nrows, int ncols)
{
    unsigned char *array;
    double start, end;

    start = current_time();
    array = (unsigned char *) pymalloc(nrows * ncols / 8);
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

// static bool
// reader(cmp_ctx_t *ctx, void *data, size_t limit)
// {
//     return recv(*((int *) ctx->buf), data, limit, 0);
// }

// static size_t
// writer(cmp_ctx_t *ctx, const void *data, size_t count)
// {
//     return send(*((int *) ctx->buf), data, count, 0);
// }

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
    PyObject *py_state, *py_T, *py_return = NULL, *py_t_xor_r;
    struct state *st;
    char *r, *t_xor_r;
    Py_ssize_t rlen;
    unsigned int m, secparam;

    if (!PyArg_ParseTuple(args, "OOs#II", &py_state, &py_T, &r, &rlen,
                          &m, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    assert(m % 8 == 0);
    m = m / 8;

    assert((int) rlen == (int) m);

    t_xor_r = (char *) pymalloc(sizeof(char) * m);
    if (t_xor_r == NULL)
        return NULL;

    py_return = PyTuple_New(secparam);

    for (unsigned int i = 0; i < secparam; ++i) {
        PyObject *tuple;
        char *t;
        Py_ssize_t tlen;

        (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_T, i), &t, &tlen);
        assert(tlen == m);
        memcpy(t_xor_r, t, m);
        xorarray((unsigned char *) t_xor_r, m, (unsigned char *) r, m);
        py_t_xor_r = PyString_FromStringAndSize(t_xor_r, m);

        tuple = PyTuple_New(2);
        PyTuple_SetItem(tuple, 0, PySequence_GetItem(py_T, i));
        PyTuple_SetItem(tuple, 1, py_t_xor_r);

        PyTuple_SetItem(py_return, i, tuple);
    }

    return py_return;
}

void *
py_msg_reader(void *msgs, int idx)
{
    return (void *) PySequence_GetItem((PyObject *) msgs, idx);
}

void
py_item_reader(void *item, int idx, void *m, ssize_t *mlen)
{
    (void) PyBytes_AsStringAndSize(PySequence_GetItem((PyObject *) item, idx),
                                   (char **) m, mlen);
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

    err = otext_iknp_send(st, py_msgs, m, msglength, secparam, s, slen, tarray,
                          py_msg_reader, py_item_reader);

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

static int
py_choice_reader(void *choices, int idx)
{
    return PyLong_AsLong(PySequence_GetItem((PyObject *) choices, idx));
}

static int
py_msg_writer(void *out, int idx, void *msg, size_t maxlength)
{
    PyObject *str = PyString_FromStringAndSize((char *) msg, maxlength);
    if (str == NULL)
        return 1;
    PyTuple_SetItem((PyObject *) out, idx, str);
    return 0;
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

    err = otext_iknp_recv(st, py_choices, nchoices, maxlength, secparam, tarray,
                          py_return, py_choice_reader, py_msg_writer);

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
