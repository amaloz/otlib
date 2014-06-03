#include <Python.h>

#include "log.h"
#include "net.h"
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <gmp.h>
#include <openssl/sha.h>

#define RANDFILE "/dev/urandom"
#define FIELD_SIZE 128          /* the field size in bytes */

static const char *ifcp1024 = "B10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C69A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C013ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD7098488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708DF1FB2BC2E4A4371";
static const char *ifcg1024 = "A4D1CBD5C3FD34126765A442EFB99905F8104DD258AC507FD6406CFF14266D31266FEA1E5C41564B777E690F5504F213160217B4B01B886A5E91547F9E2749F4D7FBD7D3B9A92EE1909D0D2263F80A76A6A24C087A091F531DBF0A0169B6A28AD662A4D18E73AFA32D779D5918D08BC8858F4DCEF97C2A24855E6EEB22B3B2E5";
static const char *ifcq1024 = "F518AA8781A8DF278ABA4E7D64B7CB9D49462353";

const char *tag = "OT-NP";

struct params {
    mpz_t p;
    mpz_t g;
    mpz_t q;
    gmp_randstate_t rnd;
};

struct state {
    struct params p;
    int sockfd;
    int serverfd;
};

static void
random_element(mpz_t out, struct params *p)
{
    mpz_urandomb(out, p->rnd, FIELD_SIZE * 8);
    mpz_mod(out, out, p->q);
    mpz_powm(out, p->g, out, p->p);
}

static void
state_destructor(PyObject *self)
{
    struct state *s;

    s = (struct state *) PyCapsule_GetPointer(self, NULL);
    if (s) {
        mpz_clears(s->p.p, s->p.g, s->p.q, NULL);
        gmp_randclear(s->p.rnd);
        if (s->sockfd != -1)
            close(s->sockfd);
        if (s->serverfd != -1)
            close(s->serverfd);
    }
}

static PyObject *
np_init(PyObject *self, PyObject *args)
{
    PyObject *py_s;
    struct state *s;
    char *host, *port;
    int isserver;

    if (!PyArg_ParseTuple(args, "ssi", &host, &port, &isserver))
        return NULL;

    s = (struct state *) pymalloc(sizeof(struct state));
    if (s == NULL)
        goto error;

    mpz_init_set_str(s->p.p, ifcp1024, 16);
    mpz_init_set_str(s->p.g, ifcg1024, 16);
    mpz_init_set_str(s->p.q, ifcq1024, 16);
    s->sockfd = -1;
    s->serverfd = -1;

    /* seed random number generator */
    {
        int error = 0, file;
        unsigned long seed;

        if ((file = open(RANDFILE, O_RDONLY)) == -1) {
            (void) fprintf(stderr, "Error opening %s\n", RANDFILE);
            error = 1;
        } else {
            if (read(file, &seed, sizeof seed) == -1) {
                (void) fprintf(stderr, "Error reading from %s\n", RANDFILE);
                (void) close(file);
                error = 1;
            }
        }
        if (error) {
            PyErr_SetString(PyExc_RuntimeError, "unable to seed randomness");
            goto error;
        } else {
            gmp_randinit_default(s->p.rnd);
            gmp_randseed_ui(s->p.rnd, seed);
            (void) close(file);
        }
    }

    if (isserver) {
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof their_addr;
        char addr[INET6_ADDRSTRLEN];

        s->serverfd = init_server(host, port);
        if (s->serverfd == -1) {
            PyErr_SetString(PyExc_RuntimeError, "server initialization failed");
            goto error;
        }
        s->sockfd = accept(s->serverfd, (struct sockaddr *) &their_addr,
                           &sin_size);
        if (s->sockfd == -1) {
            perror("accept");
            (void) close(s->serverfd);
            PyErr_SetString(PyExc_RuntimeError, "accept failed");
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  addr, sizeof addr);
        fprintf(stderr, "server: got connection from %s\n", addr);

    } else {
        s->sockfd = init_client(host, port);
        if (s->sockfd == -1) {
            PyErr_SetString(PyExc_RuntimeError, "client initialization failed");
            goto error;
        }
    }

    py_s = PyCapsule_New((void *) s, NULL, state_destructor);
    if (py_s == NULL)
        goto error;

    return py_s;

 error:
    if (s) {
        mpz_clears(s->p.p, s->p.g, s->p.q, NULL);
        gmp_randclear(s->p.rnd);
        free(s);
    }

    return NULL;
}

static PyObject *
np_send(PyObject *self, PyObject *args)
{
    mpz_t r, gr, C, pk0, pk1, pk0r, pk1r;
    char buf[FIELD_SIZE];
    char hash[SHA_DIGEST_LENGTH];
    PyObject *py_state;
    char *m0, *m1;
    int m0len, m1len, err = 0;
    struct state *s;

    if (!PyArg_ParseTuple(args, "Os#s#", &py_state, &m0, &m0len, &m1, &m1len))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    mpz_inits(r, gr, C, pk0, pk1, pk0r, pk1r, NULL);

    // choose C \in_R Zq
    random_element(C, &s->p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "C = ", C);
    // choose r \in_R Zq
    random_element(r, &s->p);
    // compute g^r
    mpz_powm(gr, s->p.g, r, s->p.p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "gr = ", gr);

    // send C to receiver
    mpz_to_array(buf, C, sizeof buf);
    if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    // send g^r to receiver
    mpz_to_array(buf, gr, sizeof buf);
    if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }

    // get pk0 from receiver
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    array_to_mpz(pk0, buf, sizeof buf);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "pk0 = ", pk0);

    // compute pk1 = C / pk0
    (void) mpz_invert(pk1, pk0, s->p.p);
    mpz_mul(pk1, pk1, C);
    mpz_mod(pk1, pk1, s->p.p);
    
    // compute pk0^r and pk1^r
    mpz_powm(pk0r, pk0, r, s->p.p);
    mpz_powm(pk1r, pk1, r, s->p.p);

    // send <H(pk0r) xor m0> to receiver
    logger_mpz(LOG_LEVEL_DEBUG, tag, "pk0r = ", pk0r);
    mpz_to_array(buf, pk0r, sizeof buf);
    (void) SHA1((unsigned char *) buf, sizeof buf, (unsigned char *) hash);
    xorarray(hash, sizeof hash, m0, m0len);
    if (pysend(s->sockfd, hash, sizeof hash, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    // send <H(pk1r) xor m1> to receiver
    logger_mpz(LOG_LEVEL_DEBUG, tag, "pk1r = ", pk1r);
    mpz_to_array(buf, pk1r, sizeof buf);
    (void) SHA1((unsigned char *) buf, sizeof buf, (unsigned char *) hash);
    xorarray(hash, sizeof hash, m1, m1len);
    if (pysend(s->sockfd, hash, sizeof hash, 0) == -1) {
        err = 1;
        goto cleanup;
    }

 cleanup:
    mpz_clears(r, gr, C, pk0, pk1, pk0r, pk1r, NULL);

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

static PyObject *
np_receive(PyObject *self, PyObject *args)
{
    mpz_t k, C, PKs, PKsp, gr;
    char buf[FIELD_SIZE];
    char h0[SHA_DIGEST_LENGTH], h1[SHA_DIGEST_LENGTH];
    struct state *s;
    PyObject *py_state, *py_return = NULL;
    int choice, err = 0;

    if (!PyArg_ParseTuple(args, "Oi", &py_state, &choice))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    mpz_inits(k, C, PKs, PKsp, gr, NULL);

    // choose random k
    mpz_urandomb(k, s->p.rnd, FIELD_SIZE * 8);
    mpz_mod(k, k, s->p.q);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "k = ", k);
    // compute PKs = g^k
    mpz_powm(PKs, s->p.g, k, s->p.p);

    // get C from sender
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    array_to_mpz(C, buf, sizeof buf);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "C = ", C);
    // get g^r from sender
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    array_to_mpz(gr, buf, sizeof buf);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "gr = ", gr);

    // compute PKsp = C / PKs
    mpz_invert(PKsp, PKs, s->p.p);
    mpz_mul(PKsp, PKsp, C);
    mpz_mod(PKsp, PKsp, s->p.p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "PKs = ", PKs);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "PKsp = ", PKsp);

    // send PK0 to sender
    mpz_to_array(buf, choice == 0 ? PKs : PKsp, sizeof buf);
    if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }

    // get H xor M0 from sender
    if (pyrecv(s->sockfd, h0, sizeof h0, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    // get H xor M1 from sender
    if (pyrecv(s->sockfd, h1, sizeof h1, 0) == -1) {
        err = 1;
        goto cleanup;
    }

    {
        mpz_t pk;
        char hash[SHA_DIGEST_LENGTH];

        mpz_init(pk);
        mpz_powm(pk, gr, k, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "pk = ", pk);
        mpz_to_array(buf, pk, sizeof buf);
        (void) SHA1((unsigned char *) buf, sizeof buf, (unsigned char *) hash);
        xorarray(hash, sizeof hash, choice == 0 ? h0 : h1,
                 choice == 0 ? sizeof h0 : sizeof h1);
        py_return = PyString_FromString(hash);
        mpz_clear(pk);
    }

 cleanup:
    mpz_clears(k, C, PKs, PKsp, gr, NULL);

    if (err)
        return NULL;
    else
        return py_return;
}

static PyMethodDef
Methods[] = {
    {"init", np_init, METH_VARARGS, "initialize Naor-Pinkas OT."},
    {"send", np_send, METH_VARARGS, "sender operation for Naor-Pinkas OT."},
    {"receive", np_receive, METH_VARARGS, "receiver operation for Naor-Pinkas OT."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_naorpinkas(void)
{
    (void) Py_InitModule("_naorpinkas", Methods);
}
