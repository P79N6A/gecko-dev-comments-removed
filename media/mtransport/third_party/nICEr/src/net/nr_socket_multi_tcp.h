
































#ifndef _nr_socket_multi_tcp_h
#define _nr_socket_multi_tcp_h

#include "nr_socket.h"




int nr_socket_multi_tcp_create(struct nr_ice_ctx_ *ctx,
  nr_transport_addr *addr,  nr_socket_tcp_type tcp_type,
  int precreated_so_count, int use_framing, int max_pending,
  nr_socket **sockp);

int nr_socket_multi_tcp_set_readable_cb(nr_socket *sock,
  NR_async_cb readable_cb,void *readable_cb_arg);

int nr_socket_multi_tcp_stun_server_connect(nr_socket *sock,
  nr_transport_addr *addr);

#endif
