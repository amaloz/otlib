#ifndef __OTLIB_OT_H__
#define __OTLIB_OT_H__

#include <stddef.h>
#include <unistd.h>

/*
 * Function for getting a pointer to the 'idx'th OT from 'msgs'
 */
typedef void * (*ot_msg_reader)(void *msgs, int idx);
/*
 * Function for reading the 'idx'th message in the OT 'item'
 */
typedef void (*ot_item_reader)(void *item, int idx, void *m, ssize_t *mlen);

#endif
