#ifndef __OTLIB_UTILS_H__
#define __OTLIB_UTILS_H__

#include "assert.h"
/* python seems to prevent us from using assert.h, so we redefine assert here */
extern void __assert_fail (const char *__assertion, const char *__file,
			   unsigned int __line, const char *__function)
    __THROW __attribute__ ((__noreturn__));
#undef assert
#define assert(expr)                                                    \
    ((expr)                                                             \
     ? __ASSERT_VOID_CAST (0)                                           \
     : (void) fprintf(stderr, "Assertion failed. %s. %s:%d. %s\n",      \
                      __STRING(expr), __FILE__, __LINE__,               \
                      __PRETTY_FUNCTION__))

#define MIN(a, b)                               \
    (a) < (b) ? (a) : (b)
#define MAX(a, b)                               \
    (a) > (b) ? (a) : (b)

double
current_time(void);

void *
pymalloc(size_t size);

int
pysend(int socket, const void *buffer, size_t length, int flags);

int
pyrecv(int socket, void *buffer, size_t length, int flags);

void
xorarray(char *a, const size_t alen, const char *b, const size_t blen);

#endif
