#include <Python.h>

#include "net.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

void *
get_in_addr(const struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *) sa)->sin_addr);
    else
        return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int
init_server(const char *addr, const char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    const int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        PyErr_SetString(PyExc_RuntimeError, gai_strerror(rv));
        return -1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL)  {
        PyErr_SetString(PyExc_RuntimeError, "failed to bind");
        return -1;
    }

    freeaddrinfo(servinfo);

    if (listen(sockfd, BACKLOG) == -1) {
        PyErr_SetString(PyExc_RuntimeError, strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int
init_client(const char *addr, const char *port)
{
    int sockfd, rv;
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        PyErr_SetString(PyExc_RuntimeError, gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "failed to connect");
        sockfd = -1;
        goto cleanup;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
              s, sizeof s);

 cleanup:
    freeaddrinfo(servinfo);
    return sockfd;
}

int
pysend(int socket, const void *buffer, size_t length, int flags)
{
    if (send(socket, buffer, length, flags) == -1) {
        PyErr_SetString(PyExc_RuntimeError, strerror(errno));
        return -1;
    }
    return 0;
}

int
pyrecv(int socket, void *buffer, size_t length, int flags)
{
    if (recv(socket, buffer, length, flags) == -1) {
        PyErr_SetString(PyExc_RuntimeError, strerror(errno));
        return -1;
    }
    return 0;
}
