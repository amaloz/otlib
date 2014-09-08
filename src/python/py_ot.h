#ifndef __OTLIB_PY_OT_H__
#define __OTLIB_PY_OT_H__

#include <unistd.h>

void *
py_ot_msg_reader(void *msgs, int idx);

void
py_ot_item_reader(void *item, int idx, void *m, ssize_t *mlen);

#endif
