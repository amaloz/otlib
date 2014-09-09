#include "ghash.h"

#include <pthread.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <x86intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

extern "C" {
    void gfmul(__m128i k, __m128i in, __m128i *out);
}

struct aes_block {
    uint64_t a;
    uint64_t b;
};

#define LEN (1<<16)
#define LEN2 1

#define zero_block _mm_setzero_si128
#define xor_block _mm_xor_si128

#define STAMP ({unsigned res; __asm__ __volatile__ ("rdtsc" : "=a"(res) : : "edx"); res;})
#define TIMES2 100
#define TIMES 10
#define NTHREADS 1
int len = LEN;
__m128i* mk;
__m128i* mmsg;
__m128i res1[NTHREADS];


// static void
// dbg(int j)
// {
//     printf("%d\n",j);
// }


__m128i
one_block()
{
    __m128i res = zero_block();
    long *r = (long *) &res;
    r[0] = 1;
    return res;
}


void
powa(__m128i k, int n, __m128i *res)
{
    int i = 0;
    __m128i tmp = one_block();
    for (i = 1; i < n; i++)
        gfmul(k, tmp, &tmp);
    *res = tmp;
}


void
ghash3(__m128i k, __m128i* msg, int len, __m128i *res)
{
    int i;
    __m128i res1, temp;
    for (i = 0; i < len; i++) {
        temp = _mm_xor_si128(msg[i], res1);
        gfmul(k, temp, &res1);
    }
    *res = res1;
}


void
ghash2(__m128i k, __m128i* msg, int len, __m128i *res)
{
    int i;
    __m128i res1,temp;
    for (i = 0; i < len; i++) {
        powa(k, len - i, &temp);
        gfmul(temp, msg[i], &temp);
        res1 = _mm_xor_si128(temp, res1);
    }
    *res = res1;
}


void
ghash(__m128i k, __m128i* msg, int len, __m128i *res)
{
    int i;
    __m128i res1, temp, temp2;
    res1 = zero_block();
    temp2 = one_block();
    for (i = 0; i < len; i++) {
        gfmul(temp2, msg[i], &temp);
        res1 =  _mm_xor_si128(temp, res1);
        gfmul(temp2, k, &temp2);
    }
    *res = res1;
}


void *
ghashworker(void *ia)
{
    int i = *((int *) ia);
    int slen = len / NTHREADS;
    int start = i*slen;
    int j, k;
    __m128i temp, temp2, tres;
    powa(*mk, start, &temp2);
    for (k = 0; k < TIMES; k++) {
        tres = zero_block();
        for (j = start; j < start + slen; j++) {
            gfmul(temp2, mmsg[j], &temp);
            tres =  xor_block(temp, tres);
            gfmul(temp2, *mk, &temp2);
        }
        res1[i] = tres;
    }
    return NULL;
}

void
ghashworker2(int start, int slen, __m128i *res)
{
    int j  =start;
    __m128i temp, temp2, r1;
    r1 = zero_block();
    powa(*mk, start, &temp2);
    for (j = start; j < start + slen; j++) {
        gfmul(temp2, mmsg[j], &temp);
        r1 =  xor_block(temp, r1);
        powa(*mk, j + 1, &temp2);
    }
    *res = r1;
}

void
ghashsplit(__m128i k, __m128i* msg, int len, __m128i *res)
{
    int i;
    // __m128i temp2;
    // temp2 = k;
    mk = &k;
    mmsg = msg;

    int tids[NTHREADS];
    pthread_t threads[NTHREADS];
    pthread_attr_t attr;

    /* Pthreads setup: initialize mutex and explicitly create threads in a
       joinable state (for portability).  Pass each thread its loop offset */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (i=0; i<NTHREADS; i++) {
        tids[i] = i;
        res1[i] = zero_block();
        pthread_create(&threads[i], &attr, ghashworker, (void *) &tids[i]);
        //     ghashworker((void *) &tids[i]);
    }
    for (i=0; i<NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    *res = zero_block();
    for (i=0;i<NTHREADS;i++)
        *res = xor_block( res1[i], *res);
}

void
ghashsplitold(__m128i k, __m128i* msg, int len, __m128i *res)
{
    int i;
    __m128i temp2;
    temp2 = k;
    mk = &k;
    mmsg = msg;

    int start = 0;
    int end = len;
    int split = (end-start)/NTHREADS;
    int slen;

    for (i = 0; i < NTHREADS; i++) {
        if (end-start > split)
            slen = split;
        else
            slen = end-start;
        if (i !=0)
            res1[i] = zero_block();//res1[i-1];
        ghashworker((void *)&i);
        start = start+slen;
    }
    p
    *res = zero_block();
    for (i = 0; i < NTHREADS; i++)
        *res = xor_block( res1[i], *res);

}

// int
// main()
// {
//     __m128i a,b,c,d;
//     __m128i* arr = (__m128i *)malloc(sizeof(__m128i)*LEN);
//     long *c2 = (long *)&c;
//     long *d2 = (long *)&d;

//     unsigned long t1,t2;
//     gfmul(a,b,&c);
//     int i,j;
//     double min = 200;
//     // ghashsplit(a,arr,LEN,&d);
//     if ((c2[0] == d2[0]) && (c2[1] == d2[1]))
//         printf("Worked!\n");

//     for (j=0;j<TIMES2;j++){
//         t1 = STAMP;
//         //	  for (i=0;i<TIMES;i++){
//         ghashsplit(a,arr,LEN,&c);
//         //	  }
//         t2 = STAMP;

//         double dx = ((t2-t1)*1.0f/(TIMES*LEN*16));
//         if (dx < min) min = dx;
//     }
//     printf("%lf",min);

// }
