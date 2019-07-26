


































#ifndef _nr_socket_buffered_stun_h
#define _nr_socket_buffered_stun_h

#include "nr_socket.h"








int nr_socket_buffered_stun_create(nr_socket *inner, int max_pending,
  nr_socket **sockp);

#endif

