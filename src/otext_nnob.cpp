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

#include "crypto.h"
#include "net.h"
#include "state.h"
#include "utils.h"

#define ERROR { err = 1; goto cleanup; }

PyObject *
otext_nnob_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_rbits, *py_ells;
    unsigned int secparam, num, seed;
    char *Ls = NULL, *bitstring = NULL, *Zs = NULL;
    unsigned int *perm = NULL, *S = NULL, *D = NULL;
    struct state *st;
    long m, err = 0;

    if (!PyArg_ParseTuple(args, "OOI", &py_state, &py_rbits, &py_ells, &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_rbits)) == -1)
        return NULL;

    num = ceil(8.0 / 3.0 * secparam);

    Ls = (char *) malloc(m * num / 8);
    if (Ls == NULL)
        ERROR;
    bitstring = (char *) malloc(m * num / 8);
    if (bitstring == NULL)
        ERROR;

    /* Step 5 */

    for (unsigned int i = 0; i < num; ++i) {
        char *L, *ell;
        Py_ssize_t len;

        (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_ells, i), &ell, &len);
        assert(len == secparam / 8);
        L = Ls + i * m / 8;

        for (int j = 0; j < m / secparam; ++j) {
            sha1_hash(L + j * secparam / 8, secparam / 8,
                      i * m / secparam + j, (unsigned char *) ell, len);
        }
    }

    /* Step 7 */

    if (pyrecv(st->sockfd, bitstring, m * num / 8, 0) == -1)
        ERROR;

    xorarray((unsigned char *) Ls, m * num / 8,
             (unsigned char *) bitstring, m * num / 8);

    /* Step 8 */

    seed = gmp_urandomb_ui(st->p.rnd, sizeof(unsigned int) * 8);
    perm = (unsigned int *) malloc(sizeof(unsigned int) * num);
    if (perm == NULL)
        ERROR;
    S = (unsigned int *) malloc(sizeof(unsigned int) * num / 2);
    if (S == NULL)
        ERROR;
    (void) random_permutation(perm, num, S, seed);
    if (pysend(st->sockfd, perm, sizeof(unsigned int) * num, 0) == -1)
        ERROR;
    if (pysend(st->sockfd, S, sizeof(unsigned int) * num / 2, 0) == -1)
        ERROR;

    /* Step 9 */

    D = (unsigned int *) malloc(sizeof(unsigned int) * num / 2);
    if (D == NULL)
        ERROR;

    /* Step 9a */

    for (unsigned int i = 0; i < num / 2; ++i) {
        unsigned int j, k, bit, pbit;

        bit = (unsigned int) PyLong_AsLong(PySequence_GetItem(py_rbits, i));
        j = perm[S[i]];
        pbit = (unsigned int) PyLong_AsLong(PySequence_GetItem(py_rbits, j));
        bit ^= pbit;
        j = i / (8 * sizeof(unsigned int));
        k = i % (8 * sizeof(unsigned int));
        D[j] |= bit << k;
    }
    if (pysend(st->sockfd, D, sizeof(unsigned int) * num / 2, 0) == -1)
        ERROR;

    /* Step 9b */

    Zs = (char *) malloc(m * num / 8);
    if (Zs == NULL)
        ERROR;

    for (unsigned int i = 0; i < num / 2; ++i) {
        char *L, *Lp, *Z;
        long xi, xpi;

        xi = PyLong_AsLong(PySequence_GetItem(py_rbits, i));
        xpi = perm[S[i]];

        L = Ls + xi * m / 8;
        Lp = Ls + xpi * m / 8;
        Z = Zs + i * m / 8;

        memcpy(Z, L, m / 8);
        xorarray((unsigned char *) Z, m / 8, (unsigned char *) Lp, m / 8);
    }

    /* Step 10 */

    /* Step 11 */

    /* Step 13 */

    /* Step 15 */

 cleanup:
    if (bitstring)
        free(bitstring);
    if (Ls)
        free(Ls);
    if (perm)
        free(perm);
    if (S)
        free(S);
    if (D)
        free(D);
    if (Zs)
        free(Zs);

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyObject *
otext_nnob_receive(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_choicestr, *py_seeds;
    unsigned int secparam, num;
    char *Lzeros = NULL, *Lones = NULL, *bitstring = NULL;
    char *choicestr;
    Py_ssize_t choicestrlen;
    struct state *st;
    long m, err = 0;

    if (!PyArg_ParseTuple(args, "OOOI", &py_state, &py_choicestr, &py_seeds,
                          &secparam))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((m = PySequence_Length(py_choicestr)) == -1)
        return NULL;
    assert(m >= secparam);

    num = ceil(8.0 / 3.0 * secparam);

    Lzeros = (char *) malloc(m * num / 8);
    if (Lzeros == NULL) {
        err = 1;
        goto cleanup;
    }
    Lones = (char *) malloc(m * num / 8);
    if (Lones == NULL) {
        err = 1;
        goto cleanup;
    }
    bitstring = (char *) malloc(m * num / 8);
    if (bitstring == NULL) {
        err = 1;
        goto cleanup;
    }

    /* Step 4 */

    for (unsigned int i = 0; i < num; ++i) {
        char *Lzero, *Lone;

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

    (void) PyBytes_AsStringAndSize(py_choicestr, &choicestr, &choicestrlen);
    assert(choicestrlen == m / 8);

    for (unsigned int i = 0; i < num; ++i) {
        char *entry, *Lzero, *Lone;

        entry = bitstring + i * m / 8;
        Lzero = Lzeros + i * m / 8;
        Lone = Lones + i * m / 8;

        /* Compute L_i^0 \xor L_i^1 \xor choicestr */

        memcpy(entry, Lzero, m / 8);
        xorarray((unsigned char *) entry, m / 8,
                 (unsigned char *) Lone, m / 8);
        xorarray((unsigned char *) entry, m / 8,
                 (unsigned char *) choicestr, m / 8);
    }

    if (pysend(st->sockfd, bitstring, m * num / 8, 0) == -1)
        ERROR;

    /* Step 8 */

    /* Step 9 */

    /* Step 10 */

    /* Step 12 */

    /* Step 14 */

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
