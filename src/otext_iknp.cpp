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

#define SHA

#if !defined AES_HW && !defined AES_SW && !defined SHA
#error one of AES_HW, AES_SW, SHA must be defined
#endif

#ifdef AES_HW
#include "aes.h"
#endif

#ifdef AES_SW
static const char *keydata = "abcdefg";
#endif

/*
 * Runs sender operations of IKNP OT extension.
 *
 * st - state information
 * msgs - vector containing messages to send
 * nmsgs - length of vector
 * maxlength - max length of each message
 * secparam - security parameter (in *bytes* not bits!)
 */
int
otext_iknp_send(struct state *st, void *msgs, long nmsgs,
                unsigned int maxlength, unsigned int secparam,
                char *s, int slen, unsigned char *array,
                ot_msg_reader msg_reader, ot_item_reader item_reader)
{
    double start, end;
    int err = 0;
    char *msg = NULL;
    double htotal = 0.0;
    double xortotal = 0.0;

    assert(slen <= SHA_DIGEST_LENGTH);

#ifdef AES_HW
    fprintf(stderr, "OTEXT-IKNP: Using AESNI\n");
#endif
#ifdef AES_SW
    fprintf(stderr, "OTEXT-IKNP: Using AES\n");
#endif
#ifdef SHA
    fprintf(stderr, "OTEXT-IKNP: Using SHA-1\n");
#endif

#ifdef AES_SW
    EVP_CIPHER_CTX enc, dec;
    aes_init((unsigned char *) keydata, strlen(keydata), &enc, &dec);
#endif

#ifdef AES_HW
    AES_KEY key;
    AES_set_encrypt_key((unsigned char *) "abcd", 128, &key);
#endif

    msg = (char *) malloc(sizeof(char) * maxlength);
    if (msg == NULL) {
        err = 1;
        goto cleanup;
    }

    start = current_time();
    for (int j = 0; j < nmsgs; ++j) {
        void *item;
        double start, end;
        unsigned char *q;

        item = msg_reader(msgs, j);
        q = &array[j * secparam];

        // fprintf(stderr, "%d ", j);

        for (int i = 0; i < 2; ++i) {
            char *m = NULL;
            ssize_t mlen;
            char hash[SHA_DIGEST_LENGTH];

            start = current_time();
            (void) memset(hash, '\0', sizeof hash);
            xorarray((unsigned char *) hash, sizeof hash, q, secparam);
            if (i == 1) {
                xorarray((unsigned char *) hash, sizeof hash,
                         (unsigned char *) s, slen);
            }
            end = current_time();
            xortotal += end - start;

            start = current_time();
#ifdef AES_HW
            AES_encrypt_message((unsigned char *) hash, sizeof hash,
                                (unsigned char *) msg, maxlength, &key);
            // AES_encrypt((unsigned char *) hash, (unsigned char *) msg, &key);
#endif
#ifdef AES_SW
            int len = sizeof hash;
            msg = (char *) aes_encrypt(&enc, (unsigned char *) hash, &len);
#endif
#ifdef SHA
            sha1_hash(msg, maxlength, j, (unsigned char *) hash, sizeof hash);
#endif
            end = current_time();
            htotal += end - start;

            item_reader(item, i, &m, &mlen);
            assert(mlen <= maxlength);
            start = current_time();
            xorarray((unsigned char *) msg, maxlength,
                     (unsigned char *) m, mlen);
            end = current_time();
            xortotal += end - start;

            // fprintf(stderr, "B ");

            if (send(st->sockfd, msg, maxlength, 0) == -1) {
                err = 1;
                goto cleanup;
            }

            // fprintf(stderr, "%d ", i);
        }

        // fprintf(stderr, "\n");
    }
 cleanup:
    if (msg)
        free(msg);
    end = current_time();
    fprintf(stderr, "hash and send: %f\n", end - start);
    fprintf(stderr, "just hash: %f\n", htotal);
    fprintf(stderr, "just xor: %f\n", xortotal);

    // sleep(1);

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
    double total = 0.0;

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
#ifdef AES_SW
    EVP_CIPHER_CTX enc, dec;
    aes_init((unsigned char *) keydata, strlen(keydata), &enc, &dec);
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
    for (int j = 0; j < nchoices; ++j) {
        int choice;

        choice = ot_choice_reader(choices, j);

        // fprintf(stderr, "%d ", j);
        fflush(stderr);

        for (int i = 0; i < 2; ++i) {
            char hash[SHA_DIGEST_LENGTH];
            unsigned char *t;

            // fprintf(stderr, "B ");

            if (recv(st->sockfd, from, maxlength, 0) == -1) {
                fprintf(stderr, "%d\n", j);
                err = 1;
                goto cleanup;
            }

            // fprintf(stderr, "%d ", i);

            t = &array[j * (secparam / 8)];
            (void) memset(hash, '\0', sizeof hash);
            (void) memcpy(hash, t, secparam / 8);

            double start, end;
            start = current_time();
#ifdef AES_HW
            AES_encrypt_message((unsigned char *) hash, sizeof hash,
                                (unsigned char *) msg, maxlength, &key);
            // AES_encrypt((unsigned char *) hash, (unsigned char *) msg, &key);
#endif
#ifdef AES_SW
            int len = sizeof hash;
            msg = (char *) aes_encrypt(&enc, (unsigned char *) hash, &len);
#endif
#ifdef SHA
            sha1_hash(msg, maxlength, j, (unsigned char *) hash,
                      SHA_DIGEST_LENGTH);
#endif
            end = current_time();
            total += end - start;

            xorarray((unsigned char *) from, maxlength,
                     (unsigned char *) msg, maxlength);
            if (i == choice) {
                ot_msg_writer(out, j, from, maxlength);
            }
        }

        // fprintf(stderr, "\n");
    }
    end = current_time();
    fprintf(stderr, "hash and receive: %f\n", end - start);
    fprintf(stderr, "just hash: %f\n", total);

 cleanup:
    if (from)
        free(from);
    if (msg)
        free(msg);

    return err;
}

