#include <Python.h>

#include <sys/socket.h>
#include <sys/time.h>

#include "state.h"
#include "utils.h"

double
current_time(void)
{
    struct timeval t;
    (void) gettimeofday(&t, NULL);
    return (double) (t.tv_sec + (double) (t.tv_usec / 1000000.0));
}

void *
pymalloc(size_t size)
{
    void *r = malloc(size);
    if (r == NULL) {
        PyErr_SetString(PyExc_MemoryError, "malloc failed");
    }
    return r;
}

int
pysend(int socket, const void *buffer, size_t length, int flags)
{
    if (send(socket, buffer, length, flags) == -1) {
        perror("send");
        PyErr_SetString(PyExc_RuntimeError, "send failed");
        return -1;
    }
    return 0;
}

int
pyrecv(int socket, void *buffer, size_t length, int flags)
{
    if (recv(socket, buffer, length, flags) == -1) {
        perror("recv");
        PyErr_SetString(PyExc_RuntimeError, "recv failed");
        return -1;
    }
    return 0;
}

void
xorarray(byte *a, const size_t alen, const byte *b, const size_t blen)
{
    size_t i;

    assert(alen >= blen);
    for (i = 0; i < blen; ++i) {
        a[i] ^= b[i];
    }
}

