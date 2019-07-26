

































#ifndef _nr_socket_turn_h
#define _nr_socket_turn_h

int nr_socket_turn_create(nr_socket *sock, int turn_state, nr_socket **sockp);
void nr_socket_turn_set_state(nr_socket *sock, int turn_state);
void nr_socket_turn_set_relay_addr(nr_socket *sock, nr_transport_addr *relay);

#endif

