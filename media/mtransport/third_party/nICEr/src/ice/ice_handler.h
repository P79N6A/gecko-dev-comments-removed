

































#ifndef _ice_h
#define _ice_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

typedef struct nr_ice_handler_vtbl_ {
  






  int (*select_pair)(void *obj,nr_ice_media_stream *stream,
int component_id, nr_ice_cand_pair **potentials,int potential_ct);

  



  int (*stream_ready)(void *obj, nr_ice_media_stream *stream);

  
  int (*stream_failed)(void *obj, nr_ice_media_stream *stream);

  


  int (*ice_completed)(void *obj, nr_ice_peer_ctx *pctx);

  
  int (*msg_recvd)(void *obj, nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, int component_id, UCHAR *msg, int len);

  
  int (*ice_checking)(void *obj, nr_ice_peer_ctx *pctx);
} nr_ice_handler_vtbl;

typedef struct nr_ice_handler_ {
  void *obj;
  nr_ice_handler_vtbl *vtbl;
} nr_ice_handler;

#ifdef __cplusplus
}
#endif 
#endif

