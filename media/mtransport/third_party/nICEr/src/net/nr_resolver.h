


































#ifndef _nr_resolver_h
#define _nr_resolver_h

#include "transport_addr.h"

#define NR_RESOLVE_PROTOCOL_STUN 1
#define NR_RESOLVE_PROTOCOL_TURN 2

typedef struct nr_resolver_resource_ {
  char *domain_name;
  UINT2 port;
  int stun_turn;
  UCHAR transport_protocol;
} nr_resolver_resource;

typedef struct nr_resolver_vtbl_ {
  int (*destroy)(void **obj);
  int (*resolve)(void *obj,
                 nr_resolver_resource *resource,
                 int (*cb)(void *cb_arg, nr_transport_addr *addr),
                 void *cb_arg,
                 void **handle);
  int (*cancel)(void *obj, void *handle);
} nr_resolver_vtbl;

typedef struct nr_resolver_ {
  void *obj;
  nr_resolver_vtbl *vtbl;
} nr_resolver;

















int nr_resolver_create_int(void *obj, nr_resolver_vtbl *vtbl,
                           nr_resolver **resolverp);
int nr_resolver_destroy(nr_resolver **resolverp);


int nr_resolver_resolve(nr_resolver *resolver,
                        nr_resolver_resource *resource,
                        int (*cb)(void *cb_arg, nr_transport_addr *addr),
                        void *cb_arg,
                        void **handle);


int nr_resolver_cancel(nr_resolver *resolver, void *handle);
#endif
