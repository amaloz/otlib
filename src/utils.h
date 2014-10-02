#ifndef __OTLIB_UTILS_H__
#define __OTLIB_UTILS_H__

#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0
#define FAILURE -1

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
ot_malloc(size_t size);

void
ot_free(void *p);

#endif
