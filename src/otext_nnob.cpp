/*
 * Implementation of maliciously secure OT extension as detailed by Frederiksen
 * and Nielsen [1], which is based on the approach of Nielsen et al. [2].
 *
 * Author: Alex J. Malozemoff <amaloz@cs.umd.edu>
 *
 * [1] "Fast and Maliciously Secure Two-Party Computation Usign the GPU."
 *     T.K. Frederiksen, J.B. Nielsen. ACNS 2013.
 *     Full version: https://eprint.iacr.org/2014/270
 *
 * [2] "A New Approach to Practical Active-Secure Two-Party Computation."
 *     J.B. Nielsen, P.S. Nordholt, C. Orlandi, S.S. Burra. CRYPTO 2012.
 *     Full version: https://eprint.iacr.org/2011/091
 */
#include "otext_nnob.h"

#include <math.h>

PyObject *
otext_nnob_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_bits, *py_ells;
    unsigned int secparam, num;
    char *Lzeros = NULL, *Lones = NULL;
    long m, err = 0;

    if (!PyArg_ParseTuple(args, "OOI", &py_state, &py_bits, &py_ells, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_msgs)) == -1)
        return NULL;

    num = ceil(8.0 / 3.0 * secparam);

    return NULL;
}

PyObject *
otext_nnob_receive(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_choicestr, *py_seeds;
    unsigned int secparam, num;
    char *Lzeros = NULL, *Lones = NULL, *bitstring;
    long m, err = 0;

    if (!PyArg_ParseTuple(args, "OOOI", &py_state, &py_choicestr, &py_seeds,
                          &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_msgs)) == -1)
        return NULL;

    num = ceil(8.0 / 3.0 * secparam);

    Lzeros = (char *) pymalloc(m * num / 8);
    if (Lzeros == NULL) {
        err = 1;
        goto cleanup;
    }
    Lones = (char *) pymalloc(m * num / 8);
    if (Lones == NULL) {
        err = 1;
        goto cleanup;
    }
    bitstring = (char *) pymalloc(m * num / 8);
    if (bitstring == NULL) {
        err = 1;
        goto cleanup;
    }

    /* Step 4 */

    for (int i = 0; i < num; ++i) {
        char *Lzero, Lone;

        Lzero = Lzeros + i * (m / 8);
        Lone = Lones + i * (m / 8);

        for (int j = 0; j < m / secparam; ++j) {
            for (int b = 0; b <= 1; ++b) {
                char *seed;
                Py_ssize_t len;

                (void) PyBytes_AsStringAndSize(PySequence_GetItem(PySequence_GetItem(py_seeds, i), b),
                                               &seed, &len);
                assert(len == secparam / 8);
                sha1_hash((b ? Lone : Lzero) + j * secparam / 8, secparam / 8,
                          i * m / secparam + j, (unsigned char *) seed, len);
            }
        }
    }

    /* Step 6 */

    for (int i = 0; i < num; ++i) {
        char *entry, *Lzero, *Lone;

        entry = bitstring + i * m / 8;
        Lzero = Lzeros + i * m / 8;
        Lone = Lones + i * m / 8;
    }

 cleanup:
    if (Lzeros)
        free(Lzeros);
    if (Lones)
        free(Lones);
    if (bitstring)
        free(bitstring);

    if (err)
        return NULL;
    else
        return NULL;
}
