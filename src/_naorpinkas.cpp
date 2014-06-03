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

    // set_log_level(LOG_LEVEL_DEBUG);

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
        (void) fprintf(stderr, "server: got connection from %s\n", addr);

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
    mpz_t r, gr, pk0;
    mpz_t *Cs = NULL, *Crs = NULL, *pks = NULL;
    char buf[FIELD_SIZE];
    char hash[SHA_DIGEST_LENGTH];
    PyObject *py_state, *py_msgs;
    long N, num_ots, err = 0;
    struct state *s;

    if (!PyArg_ParseTuple(args, "OO", &py_state, &py_msgs))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_msgs)) == -1)
        return NULL;

    if ((N = PySequence_Length(PySequence_GetItem(py_msgs, 0))) == -1)
        return NULL;

    // check to make sure all inputs to the OTs are of the same length
    for (int i = 1; i < num_ots; ++i) {
        if (PySequence_Length(PySequence_GetItem(py_msgs, i)) != N) {
            PyErr_SetString(PyExc_TypeError, "unmatched input length");
            return NULL;
        }
    }

    mpz_inits(r, gr, pk0, NULL);

    Cs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL) {
        err = 1;
        goto cleanup;
    }
    Crs = (mpz_t *) pymalloc(sizeof(mpz_t) * (N - 1));
    if (Crs == NULL) {
        err = 1;
        goto cleanup;
    }
    pks = (mpz_t *) malloc(sizeof(mpz_t) * N);
    if (pks == NULL) {
        err = 1;
        goto cleanup;
    }
    
    // choose r \in_R Zq
    random_element(r, &s->p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "r = ", r);
    // compute g^r
    mpz_powm(gr, s->p.g, r, s->p.p);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "gr = ", gr);

    for (int i = 0; i < N - 1; ++i) {
        mpz_inits(Cs[i], Crs[i], NULL);
        // choose C_i \in_R Zq
        random_element(Cs[i], &s->p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "Cs[i] = ", Cs[i]);
        // compute C_i^r
        mpz_powm(Crs[i], Cs[i], r, s->p.p);
    }

    for (int i = 0; i < N; ++i) {
        mpz_init(pks[i]);
    }

    // send g^r to receiver
    mpz_to_array(buf, gr, sizeof buf);
    if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    // send Cs to receiver
    for (int i = 0; i < N - 1; ++i) {
        mpz_to_array(buf, Cs[i], sizeof buf);
        if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
    }

    for (int ctr = 0; ctr < num_ots; ++ctr) {
        PyObject *py_input;
        Py_ssize_t mlen;
        char *m;

        py_input = PySequence_GetItem(py_msgs, ctr);
        // get pk0 from receiver
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
        array_to_mpz(pk0, buf, sizeof buf);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "pk0 = ", pk0);

        // compute pk0^r
        mpz_powm(pks[0], pk0, r, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "pks[0] = ", pks[0]);
        // compute pk_i^r
        for (int i = 0; i < N - 1; ++i) {
            (void) mpz_invert(pks[i + 1], pks[0], s->p.p);
            mpz_mul(pks[i + 1], pks[i + 1], Crs[i]);
            mpz_mod(pks[i + 1], pks[i + 1], s->p.p);
            logger_mpz(LOG_LEVEL_DEBUG, tag, "pks[i] = ", pks[i + 1]);
        }

        for (int i = 0; i < N; ++i) {
            SHA_CTX c;
            mpz_to_array(buf, pks[i], sizeof buf);
            SHA1_Init(&c);
            SHA1_Update(&c, buf, sizeof buf);
            SHA1_Update(&c, &i, sizeof i);
            SHA1_Final((unsigned char *) hash, &c);
            (void) PyBytes_AsStringAndSize(PySequence_GetItem(py_input, i),
                                           &m, &mlen);
            xorarray(hash, sizeof hash, m, mlen);
            if (pysend(s->sockfd, hash, sizeof hash, 0) == -1) {
                err = 1;
                goto cleanup;
            }
        }
    }

 cleanup:
    mpz_clears(r, gr, pk0, NULL);

    if (Cs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Cs[i]);
        free(Cs);
    }
    if (Crs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Crs[i]);
        free(Crs);
    }
    if (pks) {
        for (int i = 0; i < N; ++i)
            mpz_clear(pks[i]);
        free(pks);
    }

    if (err)
        return NULL;
    else
        Py_RETURN_NONE;
}

static PyObject *
np_receive(PyObject *self, PyObject *args)
{
    mpz_t k, gr, PK0, PKs, PKsr;
    mpz_t *Cs = NULL;
    char buf[FIELD_SIZE];
    struct state *s;
    PyObject *py_state, *py_choices, *py_return = NULL;
    int N, num_ots, err = 0;

    if (!PyArg_ParseTuple(args, "OiO", &py_state, &N, &py_choices))
        return NULL;

    s = (struct state *) PyCapsule_GetPointer(py_state, NULL);
    if (s == NULL)
        return NULL;

    if ((num_ots = PySequence_Length(py_choices)) == -1)
        return NULL;

    Cs = (mpz_t *) malloc(sizeof(mpz_t) * (N - 1));
    if (Cs == NULL) {
        err = 1;
        goto cleanup;
    }

    mpz_inits(k, gr, PK0, PKs, PKsr, NULL);

    for (int i = 0; i < N - 1; ++i) {
        mpz_init(Cs[i]);
    }

    // get g^r from sender
    if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
        err = 1;
        goto cleanup;
    }
    array_to_mpz(gr, buf, sizeof buf);
    logger_mpz(LOG_LEVEL_DEBUG, tag, "gr = ", gr);

    // get Cs from sender
    for (int i = 0; i < N - 1; ++i) {
        if (pyrecv(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }
        array_to_mpz(Cs[i], buf, sizeof buf);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "Cs[i] = ", Cs[i]);
    }

    py_return = PyTuple_New(num_ots);

    for (int ctr = 0; ctr < num_ots; ++ctr) {
        long choice;

        choice = PyLong_AsLong(PySequence_GetItem(py_choices, ctr));
        // choose random k
        mpz_urandomb(k, s->p.rnd, FIELD_SIZE * 8);
        mpz_mod(k, k, s->p.q);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "k = ", k);
        // compute PKs = g^k
        mpz_powm(PKs, s->p.g, k, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "PKs = ", PKs);

        if (choice == 0) {
            mpz_set(PK0, PKs);
        } else {
            (void) mpz_invert(PK0, PKs, s->p.p);
            mpz_mul(PK0, PK0, Cs[choice - 1]);
            mpz_mod(PK0, PK0, s->p.p);
        }
        logger_mpz(LOG_LEVEL_DEBUG, tag, "PK0 = ", PK0);

        // compute (g^r)^k = PKs^r
        mpz_powm(PKsr, gr, k, s->p.p);
        logger_mpz(LOG_LEVEL_DEBUG, tag, "PKsr = ", PKsr);

        // send PK0 to sender
        mpz_to_array(buf, PK0, sizeof buf);
        if (pysend(s->sockfd, buf, sizeof buf, 0) == -1) {
            err = 1;
            goto cleanup;
        }

        for (int i = 0; i < N; ++i) {
            char h[SHA_DIGEST_LENGTH], out[SHA_DIGEST_LENGTH];
            SHA_CTX c;
            // get H xor M0 from sender
            if (pyrecv(s->sockfd, h, sizeof h, 0) == -1) {
                err = 1;
                goto cleanup;
            }
            mpz_to_array(buf, PKsr, sizeof buf);
            (void) SHA1_Init(&c);
            (void) SHA1_Update(&c, buf, sizeof buf);
            (void) SHA1_Update(&c, &i, sizeof i);
            (void) SHA1_Final((unsigned char *) out, &c);
            xorarray(out, sizeof out, h, sizeof h);
            if (i == choice) {
                PyTuple_SetItem(py_return, ctr, PyBytes_FromString(out));
            }
        }
    }

 cleanup:
    mpz_clears(k, gr, PK0, PKs, PKsr, NULL);

    if (Cs) {
        for (int i = 0; i < N - 1; ++i)
            mpz_clear(Cs[i]);
        free(Cs);
    }

    // TODO: clean up py_return if error

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
