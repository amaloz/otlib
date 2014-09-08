#ifndef __OTLIB_OT_NP_H__
#define __OTLIB_OT_NP_H__

#include "ot.h"
#include "state.h"

int
ot_np_send(struct state *st, void *msgs, int msglength, int num_ots, int N,
           ot_msg_reader ot_msg_reader, ot_item_reader ot_item_reader);

#endif
