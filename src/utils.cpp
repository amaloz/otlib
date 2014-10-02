#include "utils.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <wmmintrin.h>

#include "state.h"

double
current_time(void)
{
    struct timeval t;
    (void) gettimeofday(&t, NULL);
    return (double) (t.tv_sec + (double) (t.tv_usec / 1000000.0));
}

void *
ot_malloc(size_t size)
{
    return _mm_malloc(size, 16);
}

void
ot_free(void *p)
{
    return _mm_free(p);
}
