

































#ifndef _nr_socket_turn_h
#define _nr_socket_turn_h





int nr_socket_turn_create(nr_socket *sock, nr_socket **sockp);
int nr_socket_turn_set_ctx(nr_socket *sock, nr_turn_client_ctx *ctx);

#endif

