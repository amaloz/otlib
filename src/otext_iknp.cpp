/*
 * Implementation if semi-honest OT extension as detailed by Ishai et al. [1].
 *
 * Author: Alex J. Malozemoff <amaloz@cs.umd.edu>
 *
 * [1] "Extending Oblivious Transfer Efficiently."
 *     Y. Ishai, J. Kilian, K. Nissim, E. Petrank. CRYPTO 2003.
 */
#include "otext_iknp.h"
#include "ot.h"

#include "crypto.h"
#include "net.h"
#include "state.h"
#include "utils.h"

#include <gmp.h>
#include <openssl/sha.h>
#include <string.h>

#include "cmp/cmp.h"

#define SHA

#if !defined AES_HW && !defined AES_SW && !defined SHA
#error one of AES_HW, AES_SW, SHA must be defined
#endif

#ifdef AES_HW
#include "aes.h"
#endif

int
otext_iknp_send(struct state *st, void *msgs, long nmsgs,
                unsigned int msglength, unsigned int secparam,
                char *s, int slen, unsigned char *array,
                ot_msg_reader msg_reader, ot_item_reader item_reader)
{
    double start, end;
    // cmp_ctx_t cmp;
    int err = 0;
    char *msg = NULL;

#ifdef AES_HW
    fprintf(stderr, "OTEXT-IKNP: Using AESNI\n");
#endif
#ifdef AES_SW
    fprintf(stderr, "OTEXT-IKNP: Using AES\n");
#endif
#ifdef SHA
    fprintf(stderr, "OTEXT-IKNP: Using SHA-1\n");
#endif

#ifdef AES_HW
    AES_KEY key;
    AES_set_encrypt_key((unsigned char *) "abcd", 128, &key);
#endif

    start = current_time();
    // cmp_init(&cmp, &st->sockfd, reader, writer);
    msg = (char *) malloc(sizeof(char) * msglength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }
    for (int j = 0; j < nmsgs; ++j) {
        void *item;

        item = msg_reader(msgs, j);

        for (int i = 0; i < 2; ++i) {
            char *m = NULL;
            unsigned char *q;
            ssize_t mlen;
            char hash[SHA_DIGEST_LENGTH];

            q = &array[j * (secparam / 8)];
            assert(slen <= (int) sizeof hash);
            (void) memset(hash, '\0', sizeof hash);
            xorarray((unsigned char *) hash, sizeof hash, q, secparam / 8);
            if (i == 1) {
                xorarray((unsigned char *) hash, sizeof hash,
                         (unsigned char *) s, slen);
            }
#ifdef AES_HW
            AES_encrypt((unsigned char *) hash, (unsigned char *) msg, &key);
#endif

#ifdef SHA
            sha1_hash(msg, msglength, j,
                      (unsigned char *) hash, SHA_DIGEST_LENGTH);
#endif

            item_reader(item, i, &m, &mlen);
            assert(mlen <= msglength);
            xorarray((unsigned char *) msg, msglength,
                     (unsigned char *) m, mlen);

            // if (!cmp_write_bin(&cmp, msg, msglength)) {
            //     PyErr_SetString(PyExc_RuntimeError, cmp_strerror(&cmp));
            //     err = 1;
            //     goto cleanup;
            // }
            if (send(st->sockfd, msg, msglength, 0) == -1) {
                err = 1;
                goto cleanup;
            }
        }
    }
 cleanup:
    if (msg)
        free(msg);
    end = current_time();
    fprintf(stderr, "hash and send: %f\n", end - start);

    return err;
}

int
otext_iknp_recv(struct state *st, void *choices, long nchoices,
                size_t maxlength, unsigned int secparam,
                unsigned char *array, void *out,
                ot_choice_reader ot_choice_reader, ot_msg_writer ot_msg_writer)
{
    char *from = NULL, *msg = NULL;
    double start, end;
    int err = 0;

#ifdef AES_HW
    AES_KEY key;
    AES_set_encrypt_key((unsigned char *) "abcd", 128, &key);
#endif

    from = (char *) malloc(sizeof(char) * maxlength);
    if (from == NULL) {
        err = 1;
        goto cleanup;
    }
    msg = (char *) malloc(sizeof(char) * maxlength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }
    
    start = current_time();
    // cmp_init(&cmp, &st->sockfd, reader, writer);
    for (int j = 0; j < nchoices; ++j) {
        int choice;

        choice = ot_choice_reader(choices, j);

        for (int i = 0; i < 2; ++i) {
            char hash[SHA_DIGEST_LENGTH];
            unsigned char *t;

            // if (!cmp_read_bin(&cmp, from, &maxlength)) {
            //     PyErr_SetString(PyExc_RuntimeError, cmp_strerror(&cmp));
            //     fprintf(stderr, "%d\n", j);
            //     err = 1;
            //     goto cleanup;
            // }
            if (recv(st->sockfd, from, maxlength, 0) == -1) {
                fprintf(stderr, "%d\n", j);
                err = 1;
                goto cleanup;
            }

            t = &array[j * (secparam / 8)];
            (void) memset(hash, '\0', sizeof hash);
            (void) memcpy(hash, t, secparam / 8);
#ifdef AES_HW
            AES_encrypt((unsigned char *) hash, (unsigned char *) msg, &key);
#endif

#ifdef SHA
            sha1_hash(msg, maxlength, j, (unsigned char *) hash,
                      SHA_DIGEST_LENGTH);
#endif

            xorarray((unsigned char *) from, maxlength,
                     (unsigned char *) msg, maxlength);
            if (i == choice) {
                ot_msg_writer(out, j, from, maxlength);
            }
        }
    }
    end = current_time();
    fprintf(stderr, "hash and receive: %f\n", end - start);

 cleanup:
    if (from)
        free(from);
    if (msg)
        free(msg);

    return err;
}

