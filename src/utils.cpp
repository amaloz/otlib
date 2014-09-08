// #include <Python.h>
#include "utils.h"

#include <sys/socket.h>
#include <sys/time.h>

#include "state.h"

double
current_time(void)
{
    struct timeval t;
    (void) gettimeofday(&t, NULL);
    return (double) (t.tv_sec + (double) (t.tv_usec / 1000000.0));
}

// void *
// pymalloc(size_t size)
// {
//     void *r = malloc(size);
//     if (r == NULL) {
//         PyErr_SetString(PyExc_MemoryError, "malloc failed");
//     }
//     return r;
// }

