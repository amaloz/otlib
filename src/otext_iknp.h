#ifndef __OTLIB_OTEXT_IKNP_H__
#define __OTLIB_OTEXT_IKNP_H__

#include "ot.h"
#include "state.h"

int
otext_iknp_send(struct state *st, void *msgs, long nmsgs,
                unsigned int msglength, unsigned int secparam,
                char *s, int slen, unsigned char *array,
                ot_msg_reader msg_reader, ot_item_reader item_reader);

int
otext_iknp_recv(struct state *st, void *choices, long nchoices,
                size_t maxlength, unsigned int secparam,
                unsigned char *array,
                void *out,
                ot_choice_reader choice_reader, ot_msg_writer msg_writer);

#endif
