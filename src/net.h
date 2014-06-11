#ifndef __OTLIB_NET_H__
#define __OTLIB_NET_H__

#include <netinet/in.h>

#define BACKLOG 5

void *
get_in_addr(struct sockaddr *sa);

int
init_server(const char *addr, const char *port);

int
init_client(const char *addr, const char *port);

#endif
