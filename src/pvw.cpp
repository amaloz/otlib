#include <Python.h>
#include "pvw.h"

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
    mpz_t gx;
    mpz_t hx;
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
    assert(0);
}

static void
dm_ddh_setup_crs(struct dm_ddh_crs *crs, enum crs_type mode, struct params *p)
{
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

// static void
// ddh_keygen(struct ddh_pk *pk, struct ddh_sk *sk, struct params *p)
// {
//     mpz_set(pk->g, p->g);
//     find_generator(pk->h, p);
//     random_element(sk->x, p);
//     mpz_powm(pk->gx, pk->g, sk->x, p->p);
//     mpz_powm(pk->hx, pk->h, sk->x, p->p);
// }

static void
ddh_enc(struct ddh_ctxt *c, const struct ddh_pk *pk, const char *msg, 
        struct params *p)
{
    mpz_t m;

    mpz_init(m);

    encode(m, msg);
    randomize(c->u, c->v, pk->g, pk->h, pk->gx, pk->hx, p);
    mpz_mul(c->v, c->v, m);
    mpz_mod(c->v, c->v, p->p);

    mpz_clear(m);
}

static void
ddh_dec(char *m, const struct ddh_sk *sk, const struct ddh_ctxt *c,
        const struct params *p)
{
    mpz_t tmp;

    mpz_init(tmp);

    mpz_powm(tmp, c->u, sk->x, p->p);
    mpz_cdiv_q(tmp, c->v, tmp);
    decode(m, tmp);

    mpz_clear(tmp);
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
           struct dm_ddh_pk *pk, int branch, char *msg,
           struct params *p)
{
    struct ddh_pk ddh_pk;

    mpz_init_set(ddh_pk.g, branch ? crs->g1 : crs->g0);
    mpz_init_set(ddh_pk.h, branch ? crs->h1 : crs->h0);
    mpz_init_set(ddh_pk.gx, pk->g);
    mpz_init_set(ddh_pk.hx, pk->h);

    ddh_enc(ctxt, &ddh_pk, msg, p);
}

static void
dm_ddh_dec(char *m, struct ddh_sk *sk, struct ddh_ctxt *ctxt,
           const struct params *p)
{
    ddh_dec(m, sk, ctxt, p);
}

PyObject *
pvw_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs;
    unsigned int msglength;
    struct dm_ddh_crs crs;
    struct dm_ddh_pk pk;
    struct state *st;
    char *msg = NULL;

    if (!PyArg_ParseTuple(args, "OOI", &py_state, &py_msgs, &msglength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    dm_ddh_setup_crs(&crs, EXT, &st->p);
    receive_dm_ddh_pk(&pk, st);
    for (int b = 0; b < 1; ++b) {
        struct ddh_ctxt ctxt;

        dm_ddh_enc(&ctxt, &crs, &pk, b, msg, &st->p);
        send_ddh_ctxt(&ctxt, st);
    }

    Py_RETURN_NONE;
}

PyObject *
pvw_receive(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_choices, *output = NULL;
    struct dm_ddh_crs crs;
    struct dm_ddh_pk pk;
    struct ddh_sk sk;
    struct state *st;
    char *msg = NULL;
    int num_ots;
    unsigned int choice, N, msglength, err = 0;

    if (!PyArg_ParseTuple(args, "OOII", &py_state, &py_choices, &N, &msglength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;

    if (N != 2) {
        // TODO: error message
        return NULL;
    }

    if ((num_ots = PySequence_Length(py_choices)) == -1)
        return NULL;

    choice = PyLong_AsLong(PySequence_GetItem(py_choices, 0));

    dm_ddh_setup_crs(&crs, EXT, &st->p);
    dm_ddh_keygen(&pk, &sk, choice, &crs, &st->p);
    send_dm_ddh_pk(&pk, st);
    for (unsigned int b = 0; b < 1; ++b) {
        struct ddh_ctxt ctxt;
        PyObject *str;

        receive_ddh_ctxt(&ctxt, st);
        dm_ddh_dec(msg, &sk, &ctxt, &st->p);
        str = PyString_FromStringAndSize(msg, msglength);
        if (str == NULL) {
            err = 1;
            goto cleanup;
        }
        if (choice == b) {
            output = str;
        }
    }

 cleanup:

    if (err)
        return NULL;
    else
        return output;
}
