


































#ifndef _nr_proxy_tunnel_h
#define _nr_proxy_tunnel_h

#include "nr_socket.h"
#include "nr_resolver.h"
#include "nr_socket_wrapper.h"

typedef struct nr_proxy_tunnel_config_ {
  nr_resolver *resolver;
  char *proxy_host;
  UINT2 proxy_port;
} nr_proxy_tunnel_config;

int nr_proxy_tunnel_config_create(nr_proxy_tunnel_config **config);

int nr_proxy_tunnel_config_destroy(nr_proxy_tunnel_config **config);

int nr_proxy_tunnel_config_set_proxy(nr_proxy_tunnel_config *config,
                                     const char* host, UINT2 port);

int nr_proxy_tunnel_config_set_resolver(nr_proxy_tunnel_config *config,
                                        nr_resolver *resolver);

int nr_socket_proxy_tunnel_create(nr_proxy_tunnel_config *config,
                                  nr_socket *inner,
                                  nr_socket **socketpp);

int nr_socket_wrapper_factory_proxy_tunnel_create(nr_proxy_tunnel_config *config,
                                                  nr_socket_wrapper_factory **factory);

#endif
