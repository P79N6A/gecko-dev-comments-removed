

































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
#include "csi_platform.h"

#ifdef __cplusplus
#define restrict
#elif defined(WIN32)
#define restrict __restrict
#endif

typedef enum {
  TCP_TYPE_NONE=0,
  TCP_TYPE_ACTIVE,
  TCP_TYPE_PASSIVE,
  TCP_TYPE_SO,
  TCP_TYPE_MAX
} nr_socket_tcp_type;

typedef struct nr_socket_ nr_socket;

typedef struct nr_socket_vtbl_ {
  UINT4 version;   
  int (*destroy)(void **obj);
  int (*ssendto)(void *obj,const void *msg, size_t len, int flags,
    nr_transport_addr *addr);
  int (*srecvfrom)(void *obj,void * restrict buf, size_t maxlen, size_t *len, int flags,
    nr_transport_addr *addr);
  int (*getfd)(void *obj, NR_SOCKET *fd);
  int (*getaddr)(void *obj, nr_transport_addr *addrp);
  int (*connect)(void *obj, nr_transport_addr *addr);
  int (*swrite)(void *obj,const void *msg, size_t len, size_t *written);
  int (*sread)(void *obj,void * restrict buf, size_t maxlen, size_t *len);
  int (*close)(void *obj);

  
  int (*listen)(void *obj, int backlog);
  int (*accept)(void *obj, nr_transport_addr *addrp, nr_socket **sockp);
} nr_socket_vtbl;


struct nr_socket_ {
  void *obj;
  nr_socket_vtbl *vtbl;
};

typedef struct nr_socket_factory_vtbl_ {
  int (*create_socket)(void *obj, nr_transport_addr *addr, nr_socket **sockp);
  int (*destroy)(void **obj);
} nr_socket_factory_vtbl;

typedef struct nr_socket_factory_ {
  void *obj;
  nr_socket_factory_vtbl *vtbl;
} nr_socket_factory;


int nr_socket_create_int(void *obj, nr_socket_vtbl *vtbl, nr_socket **sockp);
int nr_socket_destroy(nr_socket **sockp);
int nr_socket_sendto(nr_socket *sock,const void *msg, size_t len,
  int flags,nr_transport_addr *addr);
int nr_socket_recvfrom(nr_socket *sock,void * restrict buf, size_t maxlen,
  size_t *len, int flags, nr_transport_addr *addr);
int nr_socket_getfd(nr_socket *sock, NR_SOCKET *fd);
int nr_socket_getaddr(nr_socket *sock, nr_transport_addr *addrp);
int nr_socket_close(nr_socket *sock);
int nr_socket_connect(nr_socket *sock, nr_transport_addr *addr);
int nr_socket_write(nr_socket *sock,const void *msg, size_t len, size_t *written, int flags);
int nr_socket_read(nr_socket *sock, void * restrict buf, size_t maxlen, size_t *len, int flags);
int nr_socket_listen(nr_socket *sock, int backlog);
int nr_socket_accept(nr_socket *sock, nr_transport_addr *addrp, nr_socket **sockp);

int nr_socket_factory_create_int(void *obj, nr_socket_factory_vtbl *vtbl, nr_socket_factory **factorypp);
int nr_socket_factory_destroy(nr_socket_factory **factoryp);
int nr_socket_factory_create_socket(nr_socket_factory *factory, nr_transport_addr *addr, nr_socket **sockp);

#endif

