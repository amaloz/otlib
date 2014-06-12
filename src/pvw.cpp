#include "pvw.h"

const char *tag = "OT-PVW";

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

struct dm_ddh_pk {
    mpz_t g;
    mpz_t h;
};

struct dm_ddh_sk {
    mpz_t r;
};

struct dm_ddh_crs {
    mpz_t g0;
    mpz_t g1;
    mpz_t h0;
    mpz_t h1;
};

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
ddh_keygen(struct ddh_pk *pk, struct ddh_sk *sk, struct params *p)
{
    mpz_set(pk->g, p->g);
    // TODO: choose generator h
    random_element(sk->x, p);
    mpz_powm(pk->gx, pk->g, sk->x, p->p);
    mpz_powm(pk->hx, pk->h, sk->x, p->p);
}

static void
ddh_enc(struct ddh_pk *pk, char *m, struct ddh_ctxt *c)
{
    assert(0);
}

static void
ddh_dec(struct ddh_sk *sk, struct ddh_ctxt *c, char *m)
{
    assert(0);
}

static void
dm_ddh_keygen(int sigma, struct dm_ddh_pk *pk, struct dm_ddh_sk *sk,
              const struct dm_ddh_crs *crs, struct params *p)
{

    random_element(sk->r, p);
    mpz_powm(pk->g, sigma ? crs->g1 : crs->g0, sk->r, p->p);
    mpz_powm(pk->h, sigma ? crs->h1 : crs->h0, sk->r, p->p);
}

static void
dm_ddh_enc(const struct dm_ddh_crs *crs, struct dm_ddh_pk *pk, int branch, char *msg)
{
    struct ddh_pk ddh_pk;

    mpz_init_set(ddh_pk.g, branch ? crs->g1 : crs->g0);
    mpz_init_set(ddh_pk.h, branch ? crs->h1 : crs->h0);
    mpz_init_set(ddh_pk.gx, pk->g);
    mpz_init_set(ddh_pk.hx, pk->h);

    assert(0);
}

static void
dm_ddh_dec(struct dm_ddh_sk *sk, char *c)
{
    assert(0);
}

PyObject *
pvw_send(PyObject *self, PyObject *args)
{
    PyObject *py_state, *py_msgs;
    unsigned int maxlength;

    if (!PyArg_ParseTuple(args, "OOI", &py_state, &py_msgs, &maxlength))
        return NULL;

    st = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (st == NULL)
        return NULL;
}
