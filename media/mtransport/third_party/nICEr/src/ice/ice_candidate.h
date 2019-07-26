

































#ifndef _ice_candidate_h
#define _ice_candidate_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

typedef enum {HOST=1, SERVER_REFLEXIVE, PEER_REFLEXIVE, RELAYED, CTYPE_MAX} nr_ice_candidate_type;

struct nr_ice_candidate_ {
  char *label;
  int state;
#define NR_ICE_CAND_STATE_CREATED          1
#define NR_ICE_CAND_STATE_INITIALIZING     2
#define NR_ICE_CAND_STATE_INITIALIZED      3
#define NR_ICE_CAND_STATE_FAILED           4
#define NR_ICE_CAND_PEER_CANDIDATE_UNPAIRED 9
#define NR_ICE_CAND_PEER_CANDIDATE_PAIRED   10
  struct nr_ice_ctx_ *ctx;
  nr_ice_socket *isock;               


  nr_socket *osock;                   
  nr_ice_media_stream *stream;        
  nr_ice_component *component;        
  nr_ice_candidate_type type;         
  UCHAR component_id;                 
  nr_transport_addr addr;             

  nr_transport_addr base;             
  char *foundation;                   
  UINT4 priority;                     
  nr_ice_stun_server *stun_server;
  nr_transport_addr stun_server_addr; 
  void *delay_timer;
  void *resolver_handle;

  
  union {
    struct {
      nr_stun_client_ctx *stun;
      void *stun_handle;
    } srvrflx;
    struct {
      nr_turn_client_ctx *turn;
      nr_ice_turn_server *server;
      nr_ice_candidate *srvflx_candidate;
      nr_socket *turn_sock;
      void *turn_handle;
    } relayed;
  } u;

  NR_async_cb done_cb;
  void *cb_arg;

  NR_async_cb ready_cb;
  void *ready_cb_arg;
  void *ready_cb_timer;

  TAILQ_ENTRY(nr_ice_candidate_) entry_sock;
  TAILQ_ENTRY(nr_ice_candidate_) entry_comp;
};

extern char *nr_ice_candidate_type_names[];


int nr_ice_candidate_create(struct nr_ice_ctx_ *ctx,nr_ice_component *component, nr_ice_socket *isock, nr_socket *osock, nr_ice_candidate_type ctype, nr_ice_stun_server *stun_server, UCHAR component_id, nr_ice_candidate **candp);
int nr_ice_candidate_initialize(nr_ice_candidate *cand, NR_async_cb ready_cb, void *cb_arg);
int nr_ice_candidate_process_stun(nr_ice_candidate *cand, UCHAR *msg, int len, nr_transport_addr *faddr);
int nr_ice_candidate_destroy(nr_ice_candidate **candp);
void nr_ice_candidate_destroy_cb(NR_SOCKET s, int h, void *cb_arg);
int nr_ice_format_candidate_attribute(nr_ice_candidate *cand, char *attr, int maxlen);
int nr_ice_peer_candidate_from_attribute(nr_ice_ctx *ctx,char *attr,nr_ice_media_stream *stream,nr_ice_candidate **candp);
int nr_ice_peer_peer_rflx_candidate_create(nr_ice_ctx *ctx,char *label, nr_ice_component *comp,nr_transport_addr *addr, nr_ice_candidate **candp);
int nr_ice_candidate_compute_priority(nr_ice_candidate *cand);

#ifdef __cplusplus
}
#endif 
#endif

