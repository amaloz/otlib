#ifndef __OTLIB_UTILS_H__
#define __OTLIB_UTILS_H__

#include <stdlib.h>

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

typedef unsigned char byte;

double
current_time(void);

void *
pymalloc(size_t size);

void
xorarray(byte *a, const size_t alen, const byte *b, const size_t blen);

#endif
