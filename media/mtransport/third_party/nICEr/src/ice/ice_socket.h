

































#ifndef _ice_socket_h
#define _ice_socket_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

typedef struct nr_ice_stun_ctx_ {
  int type;
#define NR_ICE_STUN_NONE    0 /* Deregistered */
#define NR_ICE_STUN_CLIENT  1
#define NR_ICE_STUN_SERVER  2
#define NR_ICE_TURN_CLIENT  3

  union {
    nr_stun_client_ctx *client;
    nr_stun_server_ctx *server;
    nr_turn_client_ctx *turn_client;
  } u;

  TAILQ_ENTRY(nr_ice_stun_ctx_) entry;
} nr_ice_stun_ctx;



typedef struct nr_ice_socket_ {
  nr_socket *sock;
  nr_ice_ctx *ctx;

  nr_ice_candidate_head candidates;
  nr_ice_component *component;

  TAILQ_HEAD(nr_ice_stun_ctx_head_,nr_ice_stun_ctx_) stun_ctxs;

  nr_stun_server_ctx *stun_server;
  void *stun_server_handle;

  STAILQ_ENTRY(nr_ice_socket_) entry;
} nr_ice_socket;

typedef STAILQ_HEAD(nr_ice_socket_head_,nr_ice_socket_) nr_ice_socket_head;

int nr_ice_socket_create(struct nr_ice_ctx_ *ctx, struct nr_ice_component_ *comp, nr_socket *nsock, nr_ice_socket **sockp);
int nr_ice_socket_destroy(nr_ice_socket **isock);
int nr_ice_socket_close(nr_ice_socket *isock);
int nr_ice_socket_register_stun_client(nr_ice_socket *sock, nr_stun_client_ctx *srv,void **handle);
int nr_ice_socket_register_stun_server(nr_ice_socket *sock, nr_stun_server_ctx *srv,void **handle);
int nr_ice_socket_register_turn_client(nr_ice_socket *sock, nr_turn_client_ctx *srv,void **handle);
int nr_ice_socket_deregister(nr_ice_socket *sock, void *handle);

#ifdef __cplusplus
}
#endif 
#endif

