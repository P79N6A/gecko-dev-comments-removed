


































#ifndef _nr_socket_wrapper_h
#define _nr_socket_wrapper_h

#include "nr_socket.h"

typedef struct nr_socket_wrapper_factory_vtbl_ {
  int (*wrap)(void *obj,
                nr_socket *socket,
                nr_socket **socketp);
  int (*destroy)(void **obj);
} nr_socket_wrapper_factory_vtbl;

typedef struct nr_socket_wrapper_factory_ {
  void *obj;
  nr_socket_wrapper_factory_vtbl *vtbl;
} nr_socket_wrapper_factory;


int nr_socket_wrapper_factory_create_int(void *obj, nr_socket_wrapper_factory_vtbl *vtbl,
                                         nr_socket_wrapper_factory **wrapperp);


int nr_socket_wrapper_factory_wrap(nr_socket_wrapper_factory *wrapper, nr_socket *inner,
                                   nr_socket **socketp);

int nr_socket_wrapper_factory_destroy(nr_socket_wrapper_factory **wrapperp);

#endif
