

































#ifndef _turn_client_ctx_h
#define _turn_client_ctx_h

typedef struct nr_turn_client_ctx_ {
  int state;
#define NR_TURN_CLIENT_STATE_INITTED         1
#define NR_TURN_CLIENT_STATE_RUNNING         2
#define NR_TURN_CLIENT_STATE_ALLOCATED       3

#define NR_TURN_CLIENT_STATE_FAILED          6
#define NR_TURN_CLIENT_STATE_TIMED_OUT       7
#define NR_TURN_CLIENT_STATE_CANCELLED       8

  int phase;
#define NR_TURN_CLIENT_PHASE_INITIALIZED          -1
#define NR_TURN_CLIENT_PHASE_ALLOCATE_REQUEST1    0
#define NR_TURN_CLIENT_PHASE_ALLOCATE_REQUEST2    1
#define NR_TURN_CLIENT_PHASE_SET_ACTIVE_DEST      2

  nr_stun_client_ctx  *stun_ctx[3];    

  char *label;
  nr_transport_addr turn_server_addr;
  nr_socket *sock;
  nr_socket *wrapping_sock;
  char *username;
  Data *password;

  nr_transport_addr relay_addr;

  NR_async_cb finished_cb;
  void *cb_arg;
} nr_turn_client_ctx;

extern int NR_LOG_TURN;

int nr_turn_client_ctx_create(char *label, nr_socket *sock, nr_socket *wsock, nr_transport_addr *turn_server, UINT4 RTO, nr_turn_client_ctx **ctxp);
int nr_turn_client_ctx_destroy(nr_turn_client_ctx **ctxp);
int nr_turn_client_allocate(nr_turn_client_ctx *ctx, char *username, Data *password, UINT4 bandwidth_kbps, UINT4 lifetime_secs, NR_async_cb finished_cb, void *cb_arg);

int nr_turn_client_process_response(nr_turn_client_ctx *ctx, UCHAR *msg, int len, nr_transport_addr *turn_server_addr);
int nr_turn_client_cancel(nr_turn_client_ctx *ctx);
int nr_turn_client_relay_indication_data(nr_socket *sock, const UCHAR *msg, size_t len, int flags, nr_transport_addr *relay_addr, nr_transport_addr *remote_addr);
int nr_turn_client_rewrite_indication_data(UCHAR *msg, size_t len, size_t *newlen, nr_transport_addr *remote_addr);


#endif

