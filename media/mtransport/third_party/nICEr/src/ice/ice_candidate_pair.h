

































#ifndef _ice_candidate_pair_h
#define _ice_candidate_pair_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif


struct nr_ice_cand_pair_ {
  nr_ice_peer_ctx *pctx;
  char codeword[5];
  char *as_string;
  int state;                          
#define NR_ICE_PAIR_STATE_FROZEN           1
#define NR_ICE_PAIR_STATE_WAITING          2
#define NR_ICE_PAIR_STATE_IN_PROGRESS      3
#define NR_ICE_PAIR_STATE_FAILED           4
#define NR_ICE_PAIR_STATE_SUCCEEDED        5
#define NR_ICE_PAIR_STATE_CANCELLED        6

  UCHAR peer_nominated;               

  UCHAR nominated;                    

  UINT8 priority;                  
  nr_ice_candidate *local;            
  nr_ice_candidate *remote;           
  char *foundation;                   

  nr_stun_client_ctx *stun_client;    
  void *stun_client_handle;

  void *stun_cb_timer;
  void *restart_role_change_cb_timer;
  void *restart_nominated_cb_timer;

  TAILQ_ENTRY(nr_ice_cand_pair_) entry;
};

int nr_ice_candidate_pair_create(nr_ice_peer_ctx *pctx, nr_ice_candidate *lcand,nr_ice_candidate *rcand,nr_ice_cand_pair **pairp);
int nr_ice_candidate_pair_unfreeze(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair);
int nr_ice_candidate_pair_start(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair);
int nr_ice_candidate_pair_set_state(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair, int state);
int nr_ice_candidate_pair_dump_state(nr_ice_cand_pair *pair, FILE *out);
int nr_ice_candidate_pair_cancel(nr_ice_peer_ctx *pctx,nr_ice_cand_pair *pair);
int nr_ice_candidate_pair_select(nr_ice_cand_pair *pair);
int nr_ice_candidate_pair_do_triggered_check(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair);
int nr_ice_candidate_pair_insert(nr_ice_cand_pair_head *head,nr_ice_cand_pair *pair);
void nr_ice_candidate_pair_restart_stun_nominated_cb(NR_SOCKET s, int how, void *cb_arg);
int nr_ice_candidate_pair_destroy(nr_ice_cand_pair **pairp);
void nr_ice_candidate_pair_role_change(nr_ice_cand_pair *pair);

#ifdef __cplusplus
}
#endif 
#endif

