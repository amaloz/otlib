#ifndef __OTLIB_LOG_H__
#define __OTLIB_LOG_H__

#include <gmp.h>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_FATAL 3

void
set_log_level(int level);

void
logger(int level, const char *tag, const char *msg);

void
logger_mpz(int level, const char *tag, const char *msg, mpz_t z);

#endif
