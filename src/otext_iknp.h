#ifndef __OTLIB_OTEXT_IKNP_H__
#define __OTLIB_OTEXT_IKNP_H__

#include <stddef.h>
#include <unistd.h>

typedef void * (*msg_reader)(void *msgs, int idx);
typedef void (*item_reader)(void *item, int idx, void *m, ssize_t *mlen);

int
otext_iknp_send(struct state *st, void *msgs, long nmsgs,
                unsigned int msglength, unsigned int secparam,
                char *s, int slen, unsigned char *array,
                msg_reader msg_reader, item_reader item_reader);

typedef int (*choice_reader)(void *choices, int idx);
typedef int (*msg_writer)(void *array, int idx, void *msg, size_t maxlength);

int
otext_iknp_recv(struct state *st, void *choices, long nchoices,
                size_t maxlength, unsigned int secparam,
                unsigned char *array,
                void *out,
                choice_reader choice_reader, msg_writer msg_writer);

#endif
