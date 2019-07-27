


































#ifndef _nr_socket_buffered_stun_h
#define _nr_socket_buffered_stun_h

#include "nr_socket.h"









typedef enum {
  TURN_TCP_FRAMING=0,
  ICE_TCP_FRAMING
} nr_framing_type;

int nr_socket_buffered_stun_create(nr_socket *inner, int max_pending,
  nr_framing_type framing_type, nr_socket **sockp);

int nr_socket_buffered_set_connected_to(nr_socket *sock,
    nr_transport_addr *remote_addr);

#endif

