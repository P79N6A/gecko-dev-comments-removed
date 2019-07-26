

































#ifndef _ice_component_h
#define _ice_component_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

struct nr_ice_component_ {
  int state;
#define NR_ICE_COMPONENT_RUNNING            1
#define NR_ICE_COMPONENT_NOMINATED          2
#define NR_ICE_COMPONENT_FAILED             3
  struct nr_ice_ctx_ *ctx;
  struct nr_ice_media_stream_ *stream;
  nr_ice_component *local_component;

  int component_id;
  nr_ice_socket_head sockets;
  nr_ice_candidate_head candidates;
  int candidate_ct;

  int valid_pairs;
  struct nr_ice_cand_pair_ *nominated; 
  struct nr_ice_cand_pair_ *active;

  int keepalive_needed;
  void *keepalive_timer;
  nr_stun_client_ctx *keepalive_ctx;

  STAILQ_ENTRY(nr_ice_component_)entry;
};

typedef STAILQ_HEAD(nr_ice_component_head_,nr_ice_component_) nr_ice_component_head;

int nr_ice_component_create(struct nr_ice_media_stream_ *stream, int component_id, nr_ice_component **componentp);
int nr_ice_component_destroy(nr_ice_component **componentp);
int nr_ice_component_initialize(struct nr_ice_ctx_ *ctx,nr_ice_component *component);
int nr_ice_component_prune_candidates(nr_ice_ctx *ctx, nr_ice_component *comp);
int nr_ice_component_pair_candidates(nr_ice_peer_ctx *pctx, nr_ice_component *lcomp,nr_ice_component *pcomp);
int nr_ice_component_nominated_pair(nr_ice_component *comp, nr_ice_cand_pair *pair);
int nr_ice_component_failed_pair(nr_ice_component *comp, nr_ice_cand_pair *pair);
int nr_ice_component_select_pair(nr_ice_peer_ctx *pctx, nr_ice_component *comp);
int nr_ice_component_set_failed(nr_ice_component *comp);
int nr_ice_component_finalize(nr_ice_component *lcomp, nr_ice_component *rcomp);

#ifdef __cplusplus
}
#endif 
#endif
