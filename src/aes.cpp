/*
 This file is part of JustGarble.

    JustGarble is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JustGarble is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JustGarble.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "aes.h"

#include <stdio.h>
#include <string.h>
#include <wmmintrin.h>

#define EXPAND_ASSIST(v1,v2,v3,v4,shuff_const,aes_const)                \
    v2 = _mm_aeskeygenassist_si128(v4,aes_const);                       \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),          \
                                         _mm_castsi128_ps(v1), 16));    \
    v1 = _mm_xor_si128(v1,v3);                                          \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),          \
                                         _mm_castsi128_ps(v1), 140));   \
    v1 = _mm_xor_si128(v1,v3);                                          \
    v2 = _mm_shuffle_epi32(v2,shuff_const);                             \
    v1 = _mm_xor_si128(v1,v2)

#define EXPAND192_STEP(idx,aes_const)                                   \
    EXPAND_ASSIST(x0,x1,x2,x3,85,aes_const);                            \
    x3 = _mm_xor_si128(x3,_mm_slli_si128 (x3, 4));                      \
    x3 = _mm_xor_si128(x3,_mm_shuffle_epi32(x0, 255));                  \
    kp[idx] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(tmp),    \
                                              _mm_castsi128_ps(x0), 68)); \
    kp[idx+1] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x0),   \
                                                _mm_castsi128_ps(x3), 78)); \
    EXPAND_ASSIST(x0,x1,x2,x3,85,(aes_const*2));                        \
    x3 = _mm_xor_si128(x3,_mm_slli_si128 (x3, 4));                      \
    x3 = _mm_xor_si128(x3,_mm_shuffle_epi32(x0, 255));                  \
    kp[idx+2] = x0; tmp = x3

static inline void
AES_128_Key_Expansion(const unsigned char *userkey, void *key)
{
    __m128i x0, x1, x2;
    __m128i *kp = (__m128i *) key;
    kp[0] = x0 = _mm_loadu_si128((__m128i *) userkey);
    x2 = _mm_setzero_si128();
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 1);
    kp[1] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 2);
    kp[2] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 4);
    kp[3] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 8);
    kp[4] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 16);
    kp[5] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 32);
    kp[6] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 64);
    kp[7] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 128);
    kp[8] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 27);
    kp[9] = x0;
    EXPAND_ASSIST(x0, x1, x2, x0, 255, 54);
    kp[10] = x0;
}

static inline void
AES_192_Key_Expansion(const unsigned char *userkey, void *key)
{
    __m128i x0, x1, x2, x3, tmp, *kp = (__m128i *) key;
    kp[0] = x0 = _mm_loadu_si128((__m128i *) userkey);
    tmp = x3 = _mm_loadu_si128((__m128i *) (userkey + 16));
    x2 = _mm_setzero_si128();
    EXPAND192_STEP(1, 1);
    EXPAND192_STEP(4, 4);
    EXPAND192_STEP(7, 16);
    EXPAND192_STEP(10, 64);
}

static inline void
AES_256_Key_Expansion(const unsigned char *userkey, void *key)
{
    __m128i x0, x1, x2, x3, *kp = (__m128i *) key;
    kp[0] = x0 = _mm_loadu_si128((__m128i *) userkey);
    kp[1] = x3 = _mm_loadu_si128((__m128i *) (userkey + 16));
    x2 = _mm_setzero_si128();
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 1);
    kp[2] = x0;
    EXPAND_ASSIST(x3, x1, x2, x0, 170, 1);
    kp[3] = x3;
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 2);
    kp[4] = x0;
    EXPAND_ASSIST(x3, x1, x2, x0, 170, 2);
    kp[5] = x3;
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 4);
    kp[6] = x0;
    EXPAND_ASSIST(x3, x1, x2, x0, 170, 4);
    kp[7] = x3;
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 8);
    kp[8] = x0;
    EXPAND_ASSIST(x3, x1, x2, x0, 170, 8);
    kp[9] = x3;
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 16);
    kp[10] = x0;
    EXPAND_ASSIST(x3, x1, x2, x0, 170, 16);
    kp[11] = x3;
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 32);
    kp[12] = x0;
    EXPAND_ASSIST(x3, x1, x2, x0, 170, 32);
    kp[13] = x3;
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 64);
    kp[14] = x0;
}

int
AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key)
{
    if (bits == 128) {
        AES_128_Key_Expansion(userKey, key);
    } else if (bits == 192) {
        AES_192_Key_Expansion(userKey, key);
    } else if (bits == 256) {
        AES_256_Key_Expansion(userKey, key);
    }
#if (OCB_KEY_LEN == 0)
    key->rounds = 6 + bits / 32;
#endif
    return 0;
}

void
AES_set_decrypt_key_fast(AES_KEY *dkey, const AES_KEY *ekey)
{
    int j = 0;
    int i = ROUNDS(ekey);
#if (OCB_KEY_LEN == 0)
    dkey->rounds = i;
#endif
    dkey->rd_key[i--] = ekey->rd_key[j++];
    while (i)
        dkey->rd_key[i--] = _mm_aesimc_si128(ekey->rd_key[j++]);
    dkey->rd_key[i] = ekey->rd_key[j];
}

int
AES_set_decrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key)
{
    AES_KEY temp_key;
    AES_set_encrypt_key(userKey, bits, &temp_key);
    AES_set_decrypt_key_fast(key, &temp_key);
    return 0;
}

int
AES_encrypt_message(const unsigned char *in, size_t inlength,
                    unsigned char *out, size_t outlength, const AES_KEY *key)
{
    const int rnds = ROUNDS(key);
    const __m128i *sched = ((__m128i *) (key->rd_key));
    char integer[16];
    __m128i in128;
    unsigned char in_short[16];

    (void) memcpy(in_short, in, 16);

    in128 = _mm_loadu_si128((__m128i *) in_short);
    in128 = _mm_xor_si128(in128, sched[0]);

    (void) memset(integer, '\0', sizeof integer);

    for (unsigned int i = 0; i < outlength / 16; ++i) {
        __m128i tmp;

        (void) memcpy(integer, (char *) &i, sizeof i);
        tmp = _mm_load_si128((__m128i *) integer);
        tmp = _mm_xor_si128(tmp, in128);
        for (int j = 1; j < rnds; ++j) {
            tmp = _mm_aesenc_si128(tmp, sched[j]);
        }
        tmp = _mm_aesenclast_si128(tmp, sched[rnds]);
        _mm_store_si128((__m128i *) (out + i * 16), tmp);
    }

    return 0;
}

void
AES_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key)
{
    int j, rnds = ROUNDS(key);
    const __m128i *sched = ((__m128i *) (key->rd_key));
    __m128i tmp = _mm_load_si128((__m128i *) in);
    tmp = _mm_xor_si128(tmp, sched[0]);
    for (j = 1; j < rnds; j++)
        tmp = _mm_aesenc_si128(tmp, sched[j]);
    tmp = _mm_aesenclast_si128(tmp, sched[j]);
    _mm_store_si128((__m128i *) out, tmp);
}

void
AES_decrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key)
{
    int j, rnds = ROUNDS(key);
    const __m128i *sched = ((__m128i *) (key->rd_key));
    __m128i tmp = _mm_load_si128((__m128i *) in);
    tmp = _mm_xor_si128(tmp, sched[0]);
    for (j = 1; j < rnds; j++)
        tmp = _mm_aesdec_si128(tmp, sched[j]);
    tmp = _mm_aesdeclast_si128(tmp, sched[j]);
    _mm_store_si128((__m128i *) out, tmp);
}

void
AES_ecb_encrypt_blks(block *blks, unsigned nblks, AES_KEY *key)
{
    unsigned int i, j, rnds = ROUNDS(key);
    const __m128i *sched = ((__m128i *) (key->rd_key));
    for (i = 0; i < nblks; ++i)
        blks[i] = _mm_xor_si128(blks[i], sched[0]);
    for (j = 1; j < rnds; ++j)
        for (i = 0; i < nblks; ++i)
            blks[i] = _mm_aesenc_si128(blks[i], sched[j]);
    for (i = 0; i < nblks; ++i)
        blks[i] = _mm_aesenclast_si128(blks[i], sched[j]);
}

void
AES_ecb_encrypt_blks_4(block *blks, AES_KEY *key)
{
    unsigned int j, rnds = ROUNDS(key);
    const __m128i *sched = ((__m128i *) (key->rd_key));
    blks[0] = _mm_xor_si128(blks[0], sched[0]);
    blks[1] = _mm_xor_si128(blks[1], sched[0]);
    blks[2] = _mm_xor_si128(blks[2], sched[0]);
    blks[3] = _mm_xor_si128(blks[3], sched[0]);

    for (j = 1; j < rnds; ++j){
        blks[0] = _mm_aesenc_si128(blks[0], sched[j]);
        blks[1] = _mm_aesenc_si128(blks[1], sched[j]);
        blks[2] = _mm_aesenc_si128(blks[2], sched[j]);
        blks[3] = _mm_aesenc_si128(blks[3], sched[j]);
    }
    blks[0] = _mm_aesenclast_si128(blks[0], sched[j]);
    blks[1] = _mm_aesenclast_si128(blks[1], sched[j]);
    blks[2] = _mm_aesenclast_si128(blks[2], sched[j]);
    blks[3] = _mm_aesenclast_si128(blks[3], sched[j]);
}

void
AES_ecb_decrypt_blks(block *blks, unsigned nblks, AES_KEY *key)
{
    unsigned i, j, rnds = ROUNDS(key);
    const __m128i *sched = ((__m128i *) (key->rd_key));
    for (i = 0; i < nblks; ++i)
        blks[i] = _mm_xor_si128(blks[i], sched[0]);
    for (j = 1; j < rnds; ++j)
        for (i = 0; i < nblks; ++i)
            blks[i] = _mm_aesdec_si128(blks[i], sched[j]);
    for (i = 0; i < nblks; ++i)
        blks[i] = _mm_aesdeclast_si128(blks[i], sched[j]);
}

