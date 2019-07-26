

































#ifndef _stun_client_ctx_h
#define _stun_client_ctx_h


typedef struct nr_stun_client_ctx_ nr_stun_client_ctx;

#include "stun.h"

typedef union nr_stun_client_params_ {

    nr_stun_client_stun_binding_request_params               stun_binding_request;
    nr_stun_client_stun_keepalive_params                     stun_keepalive;
#ifdef USE_STUND_0_96
    nr_stun_client_stun_binding_request_stund_0_96_params    stun_binding_request_stund_0_96;
#endif 

#ifdef USE_ICE
    nr_stun_client_ice_use_candidate_params                  ice_use_candidate;
    nr_stun_client_ice_binding_request_params                ice_binding_request;
#endif 

#ifdef USE_TURN
    nr_stun_client_allocate_request1_params                  allocate_request1;
    nr_stun_client_allocate_request2_params                  allocate_request2;
    nr_stun_client_send_indication_params                    send_indication;
    nr_stun_client_set_active_dest_request_params            set_active_dest_request;
    nr_stun_client_data_indication_params                    data_indication;
#endif 

} nr_stun_client_params;

typedef struct nr_stun_client_stun_binding_response_results_ {
    nr_transport_addr mapped_addr;
} nr_stun_client_stun_binding_response_results;

typedef struct nr_stun_client_stun_binding_response_stund_0_96_results_ {
    nr_transport_addr mapped_addr;
} nr_stun_client_stun_binding_response_stund_0_96_results;

#ifdef USE_ICE
typedef struct nr_stun_client_ice_use_candidate_results_ {
#ifdef WIN32  
    int dummy;
#endif
} nr_stun_client_ice_use_candidate_results;

typedef struct nr_stun_client_ice_binding_response_results_ {
    nr_transport_addr mapped_addr;
} nr_stun_client_ice_binding_response_results;
#endif 

#ifdef USE_TURN
typedef struct nr_stun_client_allocate_response1_results_ {
    char *realm;
    char *nonce;
} nr_stun_client_allocate_response1_results;

typedef struct nr_stun_client_allocate_response2_results_ {
    nr_transport_addr relay_addr;
    nr_transport_addr mapped_addr;
} nr_stun_client_allocate_response2_results;

typedef struct nr_stun_client_set_active_dest_response_results_ {
#ifdef WIN32  
    int dummy;
#endif
} nr_stun_client_set_active_dest_response_results;
#endif 

typedef union nr_stun_client_results_ {
    nr_stun_client_stun_binding_response_results              stun_binding_response;
    nr_stun_client_stun_binding_response_stund_0_96_results   stun_binding_response_stund_0_96;

#ifdef USE_ICE
    nr_stun_client_ice_use_candidate_results                  ice_use_candidate;
    nr_stun_client_ice_binding_response_results               ice_binding_response;
#endif 

#ifdef USE_TURN
    nr_stun_client_allocate_response1_results                  allocate_response1;
    nr_stun_client_allocate_response2_results                  allocate_response2;
    nr_stun_client_set_active_dest_response_results            set_active_dest_response;
#endif 
} nr_stun_client_results;

struct nr_stun_client_ctx_ {
  int state;
#define NR_STUN_CLIENT_STATE_INITTED         0
#define NR_STUN_CLIENT_STATE_RUNNING         1
#define NR_STUN_CLIENT_STATE_DONE            2
#define NR_STUN_CLIENT_STATE_FAILED          3
#define NR_STUN_CLIENT_STATE_TIMED_OUT       4
#define NR_STUN_CLIENT_STATE_CANCELLED       5

  int mode;
#define NR_STUN_CLIENT_MODE_BINDING_REQUEST_SHORT_TERM_AUTH   1
#define NR_STUN_CLIENT_MODE_BINDING_REQUEST_LONG_TERM_AUTH    2
#define NR_STUN_CLIENT_MODE_BINDING_REQUEST_NO_AUTH           3
#define NR_STUN_CLIENT_MODE_KEEPALIVE                         4
#define NR_STUN_CLIENT_MODE_BINDING_REQUEST_STUND_0_96        5
#ifdef USE_ICE
#define NR_ICE_CLIENT_MODE_USE_CANDIDATE                  10
#define NR_ICE_CLIENT_MODE_BINDING_REQUEST                11
#endif 
#ifdef USE_TURN
#define NR_TURN_CLIENT_MODE_ALLOCATE_REQUEST1             20
#define NR_TURN_CLIENT_MODE_ALLOCATE_REQUEST2             21
#define NR_TURN_CLIENT_MODE_SEND_INDICATION               22
#define NR_TURN_CLIENT_MODE_SET_ACTIVE_DEST_REQUEST       23
#define NR_TURN_CLIENT_MODE_DATA_INDICATION               24
#endif 

  char *label;
  nr_transport_addr my_addr;
  nr_transport_addr peer_addr;
  nr_socket *sock;
  nr_stun_client_params params;
  nr_stun_client_results results;
  char *nonce;
  char *realm;
  void *timer_handle;
  int request_ct;
  UINT4 rto_ms;    
  double retransmission_backoff_factor;
  UINT4 maximum_transmits;
  UINT4 final_retransmit_backoff_ms;
  int timeout_ms;
  struct timeval timer_set;
  int retry_ct;
  NR_async_cb finished_cb;
  void *cb_arg;
  nr_stun_message *request;
  nr_stun_message *response;
};

int nr_stun_client_ctx_create(char *label, nr_socket *sock, nr_transport_addr *peer, UINT4 RTO, nr_stun_client_ctx **ctxp);
int nr_stun_client_start(nr_stun_client_ctx *ctx, int mode, NR_async_cb finished_cb, void *cb_arg);
int nr_stun_client_restart(nr_stun_client_ctx *ctx);
int nr_stun_client_force_retransmit(nr_stun_client_ctx *ctx);
int nr_stun_client_reset(nr_stun_client_ctx *ctx);
int nr_stun_client_ctx_destroy(nr_stun_client_ctx **ctxp);
int nr_stun_client_process_response(nr_stun_client_ctx *ctx, UCHAR *msg, int len, nr_transport_addr *peer_addr);
int nr_stun_client_cancel(nr_stun_client_ctx *ctx);

#endif

