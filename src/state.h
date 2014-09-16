#ifndef __OTLIB_STATE_H__
#define __OTLIB_STATE_H__

#include <gmp.h>

#include "gmputils.h"

struct state {
    struct params p;
    long length;
    int sockfd;
    int serverfd;
};

extern const unsigned int field_size;

#endif
