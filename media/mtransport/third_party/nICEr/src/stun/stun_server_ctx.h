

































#ifndef _stun_server_ctx_h
#define _stun_server_ctx_h

typedef struct nr_stun_server_ctx_ nr_stun_server_ctx;
typedef struct nr_stun_server_client_  nr_stun_server_client;

#include "stun_util.h"


typedef struct nr_stun_server_request_{
  nr_transport_addr src_addr;
  nr_stun_message *request;
  nr_stun_message *response;
} nr_stun_server_request;

typedef int (*nr_stun_server_cb)(void *cb_arg, struct nr_stun_server_ctx_ *ctx,nr_socket *sock,nr_stun_server_request *req, int *dont_free, int *error);

struct nr_stun_server_client_ {
  char *label;
  char *username;
  Data password;
  nr_stun_server_cb stun_server_cb;
  void *cb_arg;
  char nonce[NR_STUN_MAX_NONCE_BYTES+1];  
  STAILQ_ENTRY(nr_stun_server_client_) entry;
};

typedef STAILQ_HEAD(nr_stun_server_client_head_, nr_stun_server_client_) nr_stun_server_client_head;

struct nr_stun_server_ctx_ {
  char *label;
  nr_socket *sock;
  nr_transport_addr my_addr;
  nr_stun_server_client_head clients;
  nr_stun_server_client *default_client;
};


int nr_stun_server_ctx_create(char *label, nr_socket *sock, nr_stun_server_ctx **ctxp);
int nr_stun_server_ctx_destroy(nr_stun_server_ctx **ctxp);
int nr_stun_server_add_client(nr_stun_server_ctx *ctx, char *client_label, char *user, Data *pass, nr_stun_server_cb cb, void *cb_arg);
int nr_stun_server_remove_client(nr_stun_server_ctx *ctx, void *cb_arg);
int nr_stun_server_add_default_client(nr_stun_server_ctx *ctx, char *ufrag, Data *pass, nr_stun_server_cb cb, void *cb_arg);
int nr_stun_server_process_request(nr_stun_server_ctx *ctx, nr_socket *sock, char *msg, int len, nr_transport_addr *peer_addr, int auth_rule);
int nr_stun_get_message_client(nr_stun_server_ctx *ctx, nr_stun_message *req, nr_stun_server_client **clnt);

#endif

