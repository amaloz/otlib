/*
 * Implementation of maliciously secure OT as detailed by Peikert et al. [1].
 * In particular, we implement the DDH protocol detailed in the paper.
 *
 * Author: Alex J. Malozemoff <amaloz@cs.umd.edu>
 *
 * [1] "A Framework for Efficient and Composable Oblivious Transfer."
 *     C. Peikert, V. Vaikuntanathan, B. Waters. CRYPTO 2008.
 */
#include "ot_pvw.h"

#include "gmputils.h"
#include "net.h"
#include "state.h"
#include "utils.h"

#include <fcntl.h>

// static const char *tag = "OT-PVW";

enum crs_type {
    EXT,
    DEC
};

struct ddh_pk {
    mpz_t g;
    mpz_t h;
    mpz_t gp;
    mpz_t hp;
};

struct ddh_sk {
    mpz_t x;
};

struct ddh_ctxt {
    mpz_t u;
    mpz_t v;
};

struct dm_ddh_crs {
    mpz_t g0;
    mpz_t h0;
    mpz_t g1;
    mpz_t h1;
};

struct dm_ddh_pk {
    mpz_t g;
    mpz_t h;
};

static int
send_dm_ddh_pk(struct dm_ddh_pk *pk, const struct state *st)
{
    char g[FIELD_SIZE], h[FIELD_SIZE];

    mpz_to_array(g, pk->g, sizeof g);
    mpz_to_array(h, pk->h, sizeof h);
    if (pysend(st->sockfd, g, sizeof g, 0) == -1) {
        return FAILURE;
    }
    if (pysend(st->sockfd, h, sizeof h, 0) == -1) {
        return FAILURE;
    }
    return SUCCESS;
}

static int
receive_dm_ddh_pk(struct dm_ddh_pk *pk, const struct state *st)
{
    char g[FIELD_SIZE], h[FIELD_SIZE];

    if (pyrecv(st->sockfd, g, sizeof g, 0) == -1)
        return FAILURE;
    if (pyrecv(st->sockfd, h, sizeof h, 0) == -1)
        return FAILURE;
    array_to_mpz(pk->g, g, sizeof g);
    array_to_mpz(pk->h, h, sizeof h);
    return SUCCESS;
}

static int
send_ddh_ctxt(struct ddh_ctxt *ctxt, const struct state *st)
{
    char u[FIELD_SIZE], v[FIELD_SIZE];

    mpz_to_array(u, ctxt->u, sizeof u);
    mpz_to_array(v, ctxt->v, sizeof v);
    if (pysend(st->sockfd, u, sizeof u, 0) == -1) {
        return FAILURE;
    }
    if (pysend(st->sockfd, v, sizeof v, 0) == -1) {
        return FAILURE;
    }
    return SUCCESS;
}

static int
receive_ddh_ctxt(struct ddh_ctxt *ctxt, const struct state *st)
{
    char u[FIELD_SIZE], v[FIELD_SIZE];

    if (pyrecv(st->sockfd, u, sizeof u, 0) == -1)
        return FAILURE;
    if (pyrecv(st->sockfd, v, sizeof v, 0) == -1)
        return FAILURE;
    array_to_mpz(ctxt->u, u, sizeof u);
    array_to_mpz(ctxt->v, v, sizeof v);
    return SUCCESS;
}

// FIXME: how should we deal with the CRS?  Run it once and hardcode the
// results?

static void
dm_ddh_setup_messy(struct dm_ddh_crs *crs, struct params *p)
{
    mpz_t x0, x1;
    int file;
    unsigned long seed;

    mpz_inits(x0, x1, NULL);

    /* fix seed of random number generator */
    gmp_randseed_ui(p->rnd, 0UL);

    find_generator(crs->g0, p);
    find_generator(crs->g1, p);
    do {
        random_element(x0, p);
        random_element(x1, p);
    } while (mpz_cmp(x0, x1) == 0);

    mpz_powm(crs->h0, crs->g0, x0, p->p);
    mpz_powm(crs->h1, crs->g1, x1, p->p);

    mpz_clears(x0, x1, NULL);

    /* re-seed random number generator */
    if ((file = open("/dev/urandom", O_RDONLY)) == -1) {
        (void) fprintf(stderr, "Error opening /dev/urandom\n");
    } else {
        if (read(file, &seed, sizeof seed) == -1) {
            (void) fprintf(stderr, "Error reading from /dev/urandom\n");
            (void) close(file);
        }
    }
    gmp_randseed_ui(p->rnd, seed);
    (void) close(file);
}

static void
dm_ddh_setup_dec(struct dm_ddh_crs *crs, struct params *p)
{
    mpz_t x, y;
    int file;
    unsigned long seed;

    mpz_inits(x, y, NULL);

    /* fix seed of random number generator */
    gmp_randseed_ui(p->rnd, 0UL);

    find_generator(crs->g0, p);
    random_element(y, p);
    mpz_powm(crs->g1, crs->g0, y, p->p);
    mpz_powm(crs->h0, crs->g0, x, p->p);
    mpz_powm(crs->h1, crs->g1, x, p->p);

    mpz_clears(x, y, NULL);

    /* re-seed random number generator */
    if ((file = open("/dev/urandom", O_RDONLY)) == -1) {
        (void) fprintf(stderr, "Error opening /dev/urandom\n");
    } else {
        if (read(file, &seed, sizeof seed) == -1) {
            (void) fprintf(stderr, "Error reading from /dev/urandom\n");
            (void) close(file);
        }
    }
    gmp_randseed_ui(p->rnd, seed);
    (void) close(file);
}

static void
dm_ddh_crs_setup(struct dm_ddh_crs *crs, enum crs_type mode, struct params *p)
{
    mpz_inits(crs->g0, crs->h0, crs->g1, crs->h1, NULL);
    switch (mode) {
    case EXT:
        dm_ddh_setup_messy(crs, p);
        break;
    case DEC:
        dm_ddh_setup_dec(crs, p);
        break;
    }
}

static void
dm_ddh_crs_cleanup(struct dm_ddh_crs *crs)
{
    mpz_clears(crs->g0, crs->h0, crs->g1, crs->h1, NULL);
}

static void
dm_ddh_pk_setup(struct dm_ddh_pk *pk)
{
    mpz_inits(pk->g, pk->h, NULL);
}

static void
dm_ddh_pk_cleanup(struct dm_ddh_pk *pk)
{
    mpz_clears(pk->g, pk->h, NULL);
}

static void
ddh_sk_setup(struct ddh_sk *sk)
{
    mpz_init(sk->x);
}

static void
ddh_sk_cleanup(struct ddh_sk *sk)
{
    mpz_clear(sk->x);
}

static void
ddh_ctxt_setup(struct ddh_ctxt *c)
{
    mpz_inits(c->u, c->v, NULL);
}

static void
ddh_ctxt_cleanup(struct ddh_ctxt *c)
{
    mpz_clears(c->u, c->v, NULL);
}

static void
randomize(mpz_t u, mpz_t v, const mpz_t g, const mpz_t h, const mpz_t gp,
          const mpz_t hp, struct params *p)
{
    mpz_t s, t, tmp;

    mpz_inits(s, t, tmp, NULL);

    random_element(s, p);
    random_element(t, p);

    /* compute g^s h^t */
    mpz_powm(tmp, g, s, p->p);
    mpz_powm(u, h, t, p->p);
    mpz_mul(u, u, tmp);
    mpz_mod(u, u, p->p);

    /* compute gp^s hp^t */
    mpz_powm(tmp, gp, s, p->p);
    mpz_powm(v, hp, t, p->p);
    mpz_mul(v, v, tmp);
    mpz_mod(v, v, p->p);

    mpz_clears(s, t, tmp, NULL);
}

static void
ddh_enc(struct ddh_ctxt *c, const struct ddh_pk *pk, const char *msg,
        size_t msglen, struct params *p)
{
    mpz_t m;

    mpz_init(m);

    encode(m, msg, msglen, p);
    randomize(c->u, c->v, pk->g, pk->h, pk->gp, pk->hp, p);
    mpz_mul(c->v, c->v, m);
    mpz_mod(c->v, c->v, p->p);

    mpz_clear(m);
}

static char *
ddh_dec(const struct ddh_sk *sk, const struct ddh_ctxt *c,
        const struct params *p)
{
    mpz_t tmp;
    char *out;

    mpz_init(tmp);

    mpz_powm(tmp, c->u, sk->x, p->p);
    (void) mpz_invert(tmp, tmp, p->p);
    mpz_mul(tmp, tmp, c->v);
    out = decode(tmp, p);

    mpz_clear(tmp);

    return out;
}

static void
dm_ddh_keygen(struct dm_ddh_pk *pk, struct ddh_sk *sk, int sigma, 
              const struct dm_ddh_crs *crs, struct params *p)
{
    random_element(sk->x, p);
    mpz_powm(pk->g, sigma ? crs->g1 : crs->g0, sk->x, p->p);
    mpz_powm(pk->h, sigma ? crs->h1 : crs->h0, sk->x, p->p);
}

static void
dm_ddh_enc(struct ddh_ctxt *ctxt, const struct dm_ddh_crs *crs,
           struct dm_ddh_pk *pk, int branch, const char *msg,
           size_t msglen, struct params *p)
{
    struct ddh_pk ddh_pk;

    mpz_init_set(ddh_pk.g, branch ? crs->g1 : crs->g0);
    mpz_init_set(ddh_pk.h, branch ? crs->h1 : crs->h0);
    mpz_init_set(ddh_pk.gp, pk->g);
    mpz_init_set(ddh_pk.hp, pk->h);

    ddh_enc(ctxt, &ddh_pk, msg, msglen, p);
}

static char *
dm_ddh_dec(struct ddh_sk *sk, struct ddh_ctxt *ctxt,
           const struct params *p)
{
    return ddh_dec(sk, ctxt, p);
}

PyObject *
ot_pvw_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs;
    unsigned int msglength;
    struct dm_ddh_crs crs;
    struct dm_ddh_pk pk;
    struct ddh_ctxt ctxt;
    struct state *st;
    long N, num_ots;
    double start, end;

    if (!PyArg_ParseTuple(args, "OOI", &py_state, &py_msgs, &msglength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_msgs)) == -1)
        return NULL;

    if ((N = PySequence_Length(PySequence_GetItem(py_msgs, 0))) == -1)
        return NULL;

    if (N != 2) {
        PyErr_SetString(PyExc_RuntimeError, "N must be 2");
        return NULL;
    }

    start = current_time();
    dm_ddh_crs_setup(&crs, EXT, &st->p);
    end = current_time();
    fprintf(stderr, "CRS setup: %f\n", end - start);

    start = current_time();
    dm_ddh_pk_setup(&pk);
    ddh_ctxt_setup(&ctxt);

    for (int j = 0; j < num_ots; ++j) {
        PyObject *py_input;
        double start, end;

        py_input = PySequence_GetItem(py_msgs, j);

        // start = current_time();
        receive_dm_ddh_pk(&pk, st);
        // end = current_time();
        // fprintf(stderr, "receive dm ddh pk: %f\n", end - start);

        for (int b = 0; b <= 1; ++b) {
            char *m;
            Py_ssize_t mlen;

            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, b),
                                           &m, &mlen);
            assert(mlen <= msglength);
            // start = current_time();
            dm_ddh_enc(&ctxt, &crs, &pk, b, m, mlen, &st->p);
            // end = current_time();
            // fprintf(stderr, "encrypt dm ddh: %f\n", end - start);

            // start = current_time();
            send_ddh_ctxt(&ctxt, st);
            // end = current_time();
            // fprintf(stderr, "send ddh ctxt: %f\n", end - start);
        }
    }
    end = current_time();
    fprintf(stderr, "OT: %f\n", end - start);

    ddh_ctxt_cleanup(&ctxt);
    dm_ddh_pk_cleanup(&pk);
    dm_ddh_crs_cleanup(&crs);

    Py_RETURN_NONE;
}

PyObject *
ot_pvw_receive(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_choices, *py_return = NULL;
    struct dm_ddh_crs crs;
    struct dm_ddh_pk pk;
    struct ddh_sk sk;
    struct ddh_ctxt ctxt;
    struct state *st;
    int num_ots;
    unsigned int N, msglength, err = 0;
    double start, end;

    if (!PyArg_ParseTuple(args, "OOII", &py_state, &py_choices, &N, &msglength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if (N != 2) {
        PyErr_SetString(PyExc_RuntimeError, "N must be 2");
        return NULL;
    }

    if ((num_ots = PySequence_Length(py_choices)) == -1)
        return NULL;

    start = current_time();
    // FIXME: choice of mode should not be hardcoded
    dm_ddh_crs_setup(&crs, EXT, &st->p);
    end = current_time();
    fprintf(stderr, "CRS setup: %f\n", end - start);

    dm_ddh_pk_setup(&pk);
    ddh_sk_setup(&sk);
    ddh_ctxt_setup(&ctxt);

    py_return = PyTuple_New(num_ots);

    start = current_time();
    for (int j = 0; j < num_ots; ++j) {
        unsigned int choice;
        double start, end;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, j));

        // start = current_time();
        dm_ddh_keygen(&pk, &sk, choice, &crs, &st->p);
        // end = current_time();
        // fprintf(stderr, "ddh keygen: %f\n", end - start);

        // start = current_time();
        send_dm_ddh_pk(&pk, st);
        // end = current_time();
        // fprintf(stderr, "send dm ddh pk: %f\n", end - start);

        for (unsigned int b = 0; b <= 1; ++b) {
            char *msg;
            PyObject *str;

            // start = current_time();
            receive_ddh_ctxt(&ctxt, st);
            // end = current_time();
            // fprintf(stderr, "receive dm ddh ctxt: %f\n", end - start);

            // start = current_time();
            msg = dm_ddh_dec(&sk, &ctxt, &st->p);
            // end = current_time();
            // fprintf(stderr, "decrypt ddh ctxt: %f\n", end - start);

            str = PyString_FromStringAndSize(msg, msglength);
            free(msg);
            if (str == NULL) {
                err = 1;
                goto cleanup;
            }
            if (choice == b) {
                PyTuple_SetItem(py_return, j, str);
            }
        }
    }
    end = current_time();
    fprintf(stderr, "OT: %f\n", end - start);

 cleanup:
    ddh_ctxt_cleanup(&ctxt);
    ddh_sk_cleanup(&sk);
    dm_ddh_pk_cleanup(&pk);
    dm_ddh_crs_cleanup(&crs);

    if (err)
        return NULL;
    else
        return py_return;
}
