#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"

static int log_level = LOG_LEVEL_INFO;

void
set_log_level(int level)
{
    log_level = level;
}

void
logger(int level, const char *tag, const char *msg)
{
    if (level >= log_level) {
        fprintf(stderr, "[%s]: %s\n", tag, msg);
        fflush(stderr);
    }
}

void
logger_mpz(int level, const char *tag, const char *msg, mpz_t z)
{
    if (level >= log_level) {
        gmp_fprintf(stderr, "[%s]: %s%Zx\n", tag, msg, z);
        fflush(stderr);
    }
}
