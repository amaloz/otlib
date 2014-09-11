#ifndef __OTLIB_NET_H__
#define __OTLIB_NET_H__

#include <netinet/in.h>

#define BACKLOG 5

void *
get_in_addr(const struct sockaddr *sa);

int
init_server(const char *addr, const char *port);

int
init_client(const char *addr, const char *port);

int
sendall(int s, char *buf, size_t len);

int
recvall(int s, char *buf, size_t len);

/* int */
/* pysend(int socket, const void *buffer, size_t length, int flags); */

/* int */
/* pyrecv(int socket, void *buffer, size_t length, int flags); */

#endif
