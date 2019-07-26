

































#ifndef _nr_socket_h
#define _nr_socket_h

#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

#include "transport_addr.h"

#ifdef __cplusplus
#define restrict
#elif defined(WIN32)
#define restrict __restrict
#endif

typedef struct nr_socket_vtbl_ {
  int (*destroy)(void **obj);
  int (*ssendto)(void *obj,const void *msg, size_t len, int flags,
    nr_transport_addr *addr);
  int (*srecvfrom)(void *obj,void * restrict buf, size_t maxlen, size_t *len, int flags,
    nr_transport_addr *addr);
  int (*getfd)(void *obj, NR_SOCKET *fd);
  int (*getaddr)(void *obj, nr_transport_addr *addrp);
  int (*close)(void *obj);
} nr_socket_vtbl;

typedef struct nr_socket_ {
  void *obj;
  nr_socket_vtbl *vtbl;
} nr_socket;



int nr_socket_create_int(void *obj, nr_socket_vtbl *vtbl, nr_socket **sockp);
int nr_socket_destroy(nr_socket **sockp);
int nr_socket_sendto(nr_socket *sock,const void *msg, size_t len,
  int flags,nr_transport_addr *addr);
int nr_socket_recvfrom(nr_socket *sock,void * restrict buf, size_t maxlen,
  size_t *len, int flags, nr_transport_addr *addr);
int nr_socket_getfd(nr_socket *sock, NR_SOCKET *fd);
int nr_socket_getaddr(nr_socket *sock, nr_transport_addr *addrp);
int nr_socket_close(nr_socket *sock);

#endif

