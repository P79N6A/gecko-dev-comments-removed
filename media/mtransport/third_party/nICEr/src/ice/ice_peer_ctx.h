

































#ifndef _ice_peer_ctx_h
#define _ice_peer_ctx_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

struct nr_ice_peer_ctx_ {
  int state;
#define NR_ICE_PEER_STATE_UNPAIRED 1
#define NR_ICE_PEER_STATE_PAIRED   2

  char *label;
  nr_ice_ctx *ctx;
  nr_ice_handler *handler;

  UCHAR controlling; 
  UCHAR controlling_conflict_resolved;
  UINT8 tiebreaker;

  char *peer_ufrag;
  char *peer_pwd;
  int peer_lite;
  int peer_ice_mismatch;

  nr_ice_media_stream_head peer_streams;
  int active_streams;
  int waiting_pairs;

  void *done_cb_timer;
  UCHAR reported_done;
  void *trickle_grace_period_timer;

  STAILQ_ENTRY(nr_ice_peer_ctx_) entry;
};

typedef STAILQ_HEAD(nr_ice_peer_ctx_head_, nr_ice_peer_ctx_) nr_ice_peer_ctx_head;

int nr_ice_peer_ctx_create(nr_ice_ctx *ctx, nr_ice_handler *handler,char *label, nr_ice_peer_ctx **pctxp);
int nr_ice_peer_ctx_destroy(nr_ice_peer_ctx **pctxp);
int nr_ice_peer_ctx_parse_stream_attributes(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, char **attrs, int attr_ct);
int nr_ice_peer_ctx_find_pstream(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, nr_ice_media_stream **pstreamp);
int nr_ice_peer_ctx_parse_trickle_candidate(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, char *cand);

int nr_ice_peer_ctx_pair_candidates(nr_ice_peer_ctx *pctx);
int nr_ice_peer_ctx_parse_global_attributes(nr_ice_peer_ctx *pctx, char **attrs, int attr_ct);
int nr_ice_peer_ctx_start_checks(nr_ice_peer_ctx *pctx);
int nr_ice_peer_ctx_start_checks2(nr_ice_peer_ctx *pctx, int allow_non_first);
int nr_ice_peer_ctx_dump_state(nr_ice_peer_ctx *pctx,FILE *out);
int nr_ice_peer_ctx_log_state(nr_ice_peer_ctx *pctx);
int nr_ice_peer_ctx_stream_done(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream);
int nr_ice_peer_ctx_find_component(nr_ice_peer_ctx *pctx, nr_ice_media_stream *str, int component_id, nr_ice_component **compp);
int nr_ice_peer_ctx_deliver_packet_maybe(nr_ice_peer_ctx *pctx, nr_ice_component *comp, nr_transport_addr *source_addr, UCHAR *data, int len);
int nr_ice_peer_ctx_disable_component(nr_ice_peer_ctx *pctx, nr_ice_media_stream *lstream, int component_id);
int nr_ice_peer_ctx_pair_new_trickle_candidate(nr_ice_ctx *ctx, nr_ice_peer_ctx *pctx, nr_ice_candidate *cand);
void nr_ice_peer_ctx_switch_controlling_role(nr_ice_peer_ctx *pctx);

#ifdef __cplusplus
}
#endif 
#endif

