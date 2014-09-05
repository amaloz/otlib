#include "py_state.h"

#include "../net.h"
#include "../state.h"
#include "../utils.h"

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>


static const char *ifcp1024 = "B10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C69A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C013ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD7098488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708DF1FB2BC2E4A4371";
static const char *ifcg1024 = "A4D1CBD5C3FD34126765A442EFB99905F8104DD258AC507FD6406CFF14266D31266FEA1E5C41564B777E690F5504F213160217B4B01B886A5E91547F9E2749F4D7FBD7D3B9A92EE1909D0D2263F80A76A6A24C087A091F531DBF0A0169B6A28AD662A4D18E73AFA32D779D5918D08BC8858F4DCEF97C2A24855E6EEB22B3B2E5";
static const char *ifcq1024 = "F518AA8781A8DF278ABA4E7D64B7CB9D49462353";

#define RANDFILE "/dev/urandom"

static int
state_initialize(struct state *s, long length)
{
    int error = 0, file;
    unsigned long seed;

    mpz_init_set_str(s->p.p, ifcp1024, 16);
    mpz_init_set_str(s->p.g, ifcg1024, 16);
    mpz_init_set_str(s->p.q, ifcq1024, 16);
    s->sockfd = -1;
    s->serverfd = -1;
    s->length = length;

    /* seed random number generator */
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
    } else {
        fprintf(stderr, "Seed = %lu\n", seed);
        gmp_randinit_default(s->p.rnd);
        gmp_randseed_ui(s->p.rnd, seed);
        (void) close(file);
    }
    return error;
}

static void
state_cleanup(struct state *s)
{
    mpz_clears(s->p.p, s->p.g, s->p.q, NULL);
    gmp_randclear(s->p.rnd);
    if (s->sockfd != -1)
        close(s->sockfd);
    if (s->serverfd != -1)
        close(s->serverfd);
    free(s);
}

static void
state_destructor(PyObject *self)
{
    struct state *s;

    s = (struct state *) PyCapsule_GetPointer(self, NULL);
    if (s) {
        state_cleanup(s);
    }
}

PyObject *
py_state_init(PyObject *self, PyObject *args)
{
    struct state *st;
    char *host, *port;
    int length, isserver;

    if (!PyArg_ParseTuple(args, "ssii", &host, &port, &length, &isserver))
        return NULL;

    st = (struct state *) pymalloc(sizeof(struct state));
    if (st == NULL)
        goto error;

    if (state_initialize(st, length) == 1)
        goto error;
    if (isserver) {
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof their_addr;
        char addr[INET6_ADDRSTRLEN];

        st->serverfd = init_server(host, port);
        if (st->serverfd == -1) {
            PyErr_SetString(PyExc_RuntimeError, "server initialization failed");
            goto error;
        }
        st->sockfd = accept(st->serverfd, (struct sockaddr *) &their_addr,
                           &sin_size);
        if (st->sockfd == -1) {
            perror("accept");
            (void) close(st->serverfd);
            PyErr_SetString(PyExc_RuntimeError, "accept failed");
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  addr, sizeof addr);
        (void) fprintf(stderr, "server: got connection from %s\n", addr);
    } else {
        st->sockfd = init_client(host, port);
        if (st->sockfd == -1) {
            PyErr_SetString(PyExc_RuntimeError, "client initialization failed");
            goto error;
        }
    }

    {
        PyObject *py_st;
        py_st = PyCapsule_New((void *) st, NULL, state_destructor);
        if (py_st == NULL)
            goto error;
        return py_st;
    }

 error:
    if (st) {
        state_cleanup(st);
    }

    return NULL;
}
