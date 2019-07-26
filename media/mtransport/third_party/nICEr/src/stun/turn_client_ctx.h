




































#ifndef _turn_client_ctx_h
#define _turn_client_ctx_h






typedef struct nr_turn_stun_ctx_ {
  struct nr_turn_client_ctx_ *tctx;
  int mode;  
  int retry_ct;
  nr_stun_client_ctx *stun;
  char *nonce;
  char *realm;
  NR_async_cb success_cb;
  NR_async_cb error_cb;

  STAILQ_ENTRY(nr_turn_stun_ctx_) entry;
} nr_turn_stun_ctx;
typedef STAILQ_HEAD(nr_turn_stun_ctx_head_, nr_turn_stun_ctx_)
    nr_turn_stun_ctx_head;


typedef struct nr_turn_permission_ {
  nr_transport_addr addr;
  nr_turn_stun_ctx *stun;
  UINT8 last_used;

  STAILQ_ENTRY(nr_turn_permission_) entry;
} nr_turn_permission;
typedef STAILQ_HEAD(nr_turn_permission_head_, nr_turn_permission_)
    nr_turn_permission_head;



typedef struct nr_turn_client_ctx_ {
  int state;
#define NR_TURN_CLIENT_STATE_INITTED         1
#define NR_TURN_CLIENT_STATE_ALLOCATING      2
#define NR_TURN_CLIENT_STATE_ALLOCATED       3
#define NR_TURN_CLIENT_STATE_FAILED          4
#define NR_TURN_CLIENT_STATE_CANCELLED       5

  char *label;
  nr_socket *sock;

  char *username;
  Data *password;
  char *nonce;
  char *realm;

  nr_transport_addr turn_server_addr;
  nr_transport_addr relay_addr;
  nr_transport_addr mapped_addr;

  nr_turn_stun_ctx_head stun_ctxs;
  nr_turn_permission_head permissions;

  NR_async_cb finished_cb;
  void *cb_arg;

  void *connected_timer_handle;
  void *refresh_timer_handle;
} nr_turn_client_ctx;

extern int NR_LOG_TURN;

int nr_turn_client_ctx_create(const char *label, nr_socket *sock,
                              const char *username, Data *password,
                              nr_transport_addr *addr,
                              nr_turn_client_ctx **ctxp);
int nr_turn_client_ctx_destroy(nr_turn_client_ctx **ctxp);
int nr_turn_client_allocate(nr_turn_client_ctx *ctx,
                            NR_async_cb finished_cb, void *cb_arg);
int nr_turn_client_get_relayed_address(nr_turn_client_ctx *ctx,
                                       nr_transport_addr *relayed_address);
int nr_turn_client_get_mapped_address(nr_turn_client_ctx *ctx,
                                      nr_transport_addr *mapped_address);
int nr_turn_client_process_response(nr_turn_client_ctx *ctx,
                                    UCHAR *msg, int len,
                                    nr_transport_addr *turn_server_addr);
int nr_turn_client_cancel(nr_turn_client_ctx *ctx);
int nr_turn_client_send_indication(nr_turn_client_ctx *ctx,
                                   const UCHAR *msg, size_t len,
                                   int flags, nr_transport_addr *remote_addr);
int nr_turn_client_parse_data_indication(nr_turn_client_ctx *ctx,
                                         nr_transport_addr *source_addr,
                                         UCHAR *msg, size_t len,
                                         UCHAR *newmsg, size_t *newlen,
                                         size_t newsize,
                                         nr_transport_addr *remote_addr);
#endif

