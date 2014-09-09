#ifndef _GHASH_
#define _GHASH_

#include <wmmintrin.h>

void ghash (__m128i k, __m128i* msg, int len, __m128i *res);

#endif
