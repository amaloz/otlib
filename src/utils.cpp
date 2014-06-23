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

void
xorarray(byte *a, const size_t alen, const byte *b, const size_t blen)
{
    size_t i;

    assert(alen >= blen);
    for (i = 0; i < blen; ++i) {
        a[i] ^= b[i];
    }
}

