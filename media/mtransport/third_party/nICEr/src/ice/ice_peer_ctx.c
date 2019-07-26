

































static char *RCSSTRING __UNUSED__="$Id: ice_peer_ctx.c,v 1.2 2008/04/28 17:59:01 ekr Exp $";

#include <string.h>
#include <assert.h>
#include <nr_api.h>
#include "ice_ctx.h"
#include "ice_peer_ctx.h"
#include "nr_crypto.h"
#include "async_timer.h"

static void nr_ice_peer_ctx_destroy_cb(NR_SOCKET s, int how, void *cb_arg);
static int nr_ice_peer_ctx_parse_stream_attributes_int(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, nr_ice_media_stream *pstream, char **attrs, int attr_ct);
static int nr_ice_ctx_parse_candidate(nr_ice_peer_ctx *pctx, nr_ice_media_stream *pstream, char *candidate);

int nr_ice_peer_ctx_create(nr_ice_ctx *ctx, nr_ice_handler *handler,char *label, nr_ice_peer_ctx **pctxp)
  {
    int r,_status;
    nr_ice_peer_ctx *pctx=0;

    if(!(pctx=RCALLOC(sizeof(nr_ice_peer_ctx))))
      ABORT(R_NO_MEMORY);

    if(!(pctx->label=r_strdup(label)))
      ABORT(R_NO_MEMORY);

    pctx->ctx=ctx;
    pctx->handler=handler;

    
    if(ctx->flags & NR_ICE_CTX_FLAGS_LITE){
      if(pctx->peer_lite){
        r_log(LOG_ICE,LOG_ERR,"Both sides are ICE-Lite");
        ABORT(R_BAD_DATA);
      }

      pctx->controlling=0;
    }
    else{
      if(pctx->peer_lite || (ctx->flags & NR_ICE_CTX_FLAGS_OFFERER))
        pctx->controlling=1;
      else if(ctx->flags & NR_ICE_CTX_FLAGS_ANSWERER)
        pctx->controlling=0;
    }
    if(r=nr_crypto_random_bytes((UCHAR *)&pctx->tiebreaker,8))
      ABORT(r);

    STAILQ_INIT(&pctx->peer_streams);

    STAILQ_INSERT_TAIL(&ctx->peers,pctx,entry);

    *pctxp=pctx;

    _status = 0;
  abort:
    if(_status){
      nr_ice_peer_ctx_destroy_cb(0,0,pctx);
    }
    return(_status);
  }



int nr_ice_peer_ctx_parse_stream_attributes(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, char **attrs, int attr_ct)
  {
    nr_ice_media_stream *pstream=0;
    nr_ice_component *comp,*comp2;
    int r,_status;

    


    if(r=nr_ice_media_stream_create(pctx->ctx,stream->label,stream->component_ct,&pstream))
      ABORT(r);

    
    comp=STAILQ_FIRST(&stream->components);
    comp2=STAILQ_FIRST(&pstream->components);
    while(comp){
      comp2->local_component=comp;

      comp=STAILQ_NEXT(comp,entry);
      comp2=STAILQ_NEXT(comp2,entry);
    }

    pstream->local_stream=stream;
    pstream->pctx=pctx;

    if (r=nr_ice_peer_ctx_parse_stream_attributes_int(pctx,stream,pstream,attrs,attr_ct))
      ABORT(r);


    STAILQ_INSERT_TAIL(&pctx->peer_streams,pstream,entry);

    _status=0;
  abort:
    return(_status);
  }

static int nr_ice_peer_ctx_parse_stream_attributes_int(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, nr_ice_media_stream *pstream, char **attrs, int attr_ct)
  {
    int r;
    int i;

    for(i=0;i<attr_ct;i++){
      if(!strncmp(attrs[i],"ice-",4)){
        if(r=nr_ice_peer_ctx_parse_media_stream_attribute(pctx,pstream,attrs[i])) {
          r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) specified bogus ICE attribute",pctx->ctx->label,pctx->label);
          continue;
        }
      }
      else if (!strncmp(attrs[i],"candidate",9)){
        if(r=nr_ice_ctx_parse_candidate(pctx,pstream,attrs[i])) {
          r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) specified bogus candidate",pctx->ctx->label,pctx->label);
          continue;
        }
      }
      else {
        r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) specified bogus attribute",pctx->ctx->label,pctx->label);
      }
    }

    
    return(0);
  }

static int nr_ice_ctx_parse_candidate(nr_ice_peer_ctx *pctx, nr_ice_media_stream *pstream, char *candidate)
  {
    nr_ice_candidate *cand=0;
    nr_ice_component *comp;
    int j;
    int r, _status;

    if(r=nr_ice_peer_candidate_from_attribute(pctx->ctx,candidate,pstream,&cand))
      ABORT(r);
    if(cand->component_id-1>=pstream->component_ct){
      r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) specified too many components",pctx->ctx->label,pctx->label);
      ABORT(R_BAD_DATA);
    }

    
    j=1;
    for(comp=STAILQ_FIRST(&pstream->components);comp;comp=STAILQ_NEXT(comp,entry)){
      if(j==cand->component_id)
        break;

      j++;
    }

    if(!comp){
      r_log(LOG_ICE,LOG_ERR,"Peer answered with more components than we offered");
      ABORT(R_BAD_DATA);
    }

    cand->component=comp;

    TAILQ_INSERT_TAIL(&comp->candidates,cand,entry_comp);

    _status=0;
 abort:
    if (_status) {
      nr_ice_candidate_destroy(&cand);
    }
    return(_status);
  }



int nr_ice_peer_ctx_parse_trickle_candidate(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, char *candidate)
  {
    

    nr_ice_media_stream *pstream;
    int r,_status;
    int needs_pairing = 0;

    pstream=STAILQ_FIRST(&pctx->peer_streams);
    while(pstream) {
      if (pstream->local_stream == stream)
        break;

      pstream = STAILQ_NEXT(pstream, entry);
    }
    if (!pstream) {
      r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) has no stream matching stream %s",pctx->ctx->label,pctx->label,stream->label);
      ABORT(R_NOT_FOUND);
    }

    switch(pstream->ice_state) {
      case NR_ICE_MEDIA_STREAM_UNPAIRED:
        break;
      case NR_ICE_MEDIA_STREAM_CHECKS_FROZEN:
      case NR_ICE_MEDIA_STREAM_CHECKS_ACTIVE:
        needs_pairing = 1;
        break;
      default:
        r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s), stream(%s) tried to trickle ICE in inappropriate state %d",pctx->ctx->label,pctx->label,stream->label,pstream->ice_state);
        ABORT(R_ALREADY);
        break;
    }

    if(r=nr_ice_ctx_parse_candidate(pctx,pstream,candidate)){
      ABORT(r);
    }

    



    if (needs_pairing) {
      if(r=nr_ice_media_stream_pair_candidates(pctx, stream, pstream)) {
        r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s), stream(%s) failed to pair trickle ICE candidates",pctx->ctx->label,pctx->label,stream->label);
        ABORT(r);
      }
    }

    












    if (!pstream->timer) {
      if(r=nr_ice_media_stream_start_checks(pctx, pstream)) {
        r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s), stream(%s) failed to start checks",pctx->ctx->label,pctx->label,stream->label);
        ABORT(r);
      }
    }

    _status=0;
 abort:
    return(_status);

  }


int nr_ice_peer_ctx_pair_candidates(nr_ice_peer_ctx *pctx)
  {
    nr_ice_media_stream *stream;
    int r,_status;

    if(STAILQ_EMPTY(&pctx->peer_streams)) {
        r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) received no media stream attribributes",pctx->ctx->label,pctx->label);
        ABORT(R_FAILED);
    }

    stream=STAILQ_FIRST(&pctx->peer_streams);
    while(stream){
      if(r=nr_ice_media_stream_pair_candidates(pctx, stream->local_stream,
        stream))
        ABORT(r);

      stream=STAILQ_NEXT(stream,entry);
    }

    _status=0;
  abort:
    return(_status);
  }

static void nr_ice_peer_ctx_destroy_cb(NR_SOCKET s, int how, void *cb_arg)
  {
    nr_ice_peer_ctx *pctx=cb_arg;
    nr_ice_media_stream *str1,*str2;

    NR_async_timer_cancel(pctx->done_cb_timer);
    RFREE(pctx->label);
    RFREE(pctx->peer_ufrag);
    RFREE(pctx->peer_pwd);

    STAILQ_FOREACH_SAFE(str1, &pctx->peer_streams, entry, str2){
      STAILQ_REMOVE(&pctx->peer_streams,str1,nr_ice_media_stream_,entry);
      nr_ice_media_stream_destroy(&str1);
    }
    assert(pctx->ctx);
    if (pctx->ctx)
      STAILQ_REMOVE(&pctx->ctx->peers, pctx, nr_ice_peer_ctx_, entry);

    RFREE(pctx);
  }

int nr_ice_peer_ctx_destroy(nr_ice_peer_ctx **pctxp)
  {

    if(!pctxp || !*pctxp)
      return(0);

    
    (*pctxp)->handler = 0;

    NR_ASYNC_SCHEDULE(nr_ice_peer_ctx_destroy_cb,*pctxp);

    *pctxp=0;

    return(0);
  }




int nr_ice_peer_ctx_start_checks(nr_ice_peer_ctx *pctx)
  {
    return nr_ice_peer_ctx_start_checks2(pctx, 0);
  }










int nr_ice_peer_ctx_start_checks2(nr_ice_peer_ctx *pctx, int allow_non_first)
  {
    int r,_status;
    nr_ice_media_stream *stream;

    stream=STAILQ_FIRST(&pctx->peer_streams);
    if(!stream)
      ABORT(R_FAILED);

    while (stream) {
      if(!TAILQ_EMPTY(&stream->check_list))
        break;

      if(!allow_non_first){
        r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) first stream has empty check list",pctx->ctx->label,pctx->label);
        ABORT(R_FAILED);
      }

      stream=STAILQ_NEXT(stream, entry);
    }

    if (!stream) {
      r_log(LOG_ICE,LOG_ERR,"ICE(%s): peer (%s) no streams with non-empty check lists",pctx->ctx->label,pctx->label);
      ABORT(R_NOT_FOUND);
    }

    if (stream->ice_state == NR_ICE_MEDIA_STREAM_CHECKS_FROZEN) {
      if(r=nr_ice_media_stream_unfreeze_pairs(pctx,stream))
        ABORT(r);
      if(r=nr_ice_media_stream_start_checks(pctx,stream))
        ABORT(r);
    }

    _status=0;
  abort:
    return(_status);
  }

#ifndef NDEBUG
int nr_ice_peer_ctx_dump_state(nr_ice_peer_ctx *pctx,FILE *out)
  {
    int r,_status;
    nr_ice_media_stream *stream;

    fprintf(out,"PEER %s STATE DUMP\n",pctx->label);
    fprintf(out,"==========================================\n");
    stream=STAILQ_FIRST(&pctx->peer_streams);
    while(stream){
      if(r=nr_ice_media_stream_dump_state(pctx,stream,out))
        ABORT(r);

      stream=STAILQ_NEXT(stream,entry);
    }
    fprintf(out,"==========================================\n");

    _status=0;
  abort:
    return(_status);
  }
#endif

static void nr_ice_peer_ctx_fire_done(NR_SOCKET s, int how, void *cb_arg)
  {
    nr_ice_peer_ctx *pctx=cb_arg;

    pctx->done_cb_timer=0;

    
    if (pctx->handler) {
      pctx->handler->vtbl->ice_completed(pctx->handler->obj, pctx);
    }
  }




int nr_ice_peer_ctx_stream_done(nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream)
  {
    int _status;
    nr_ice_media_stream *str;
    int failed=0;
    int succeeded=0;

    str=STAILQ_FIRST(&pctx->peer_streams);
    while(str){
      if(str->ice_state==NR_ICE_MEDIA_STREAM_CHECKS_COMPLETED){
        succeeded++;
      }
      else if(str->ice_state==NR_ICE_MEDIA_STREAM_CHECKS_FAILED){
        failed++;
      }
      else{
        break;
      }
      str=STAILQ_NEXT(str,entry);
    }

    if(str)
      goto done;  

    
    r_log(LOG_ICE,LOG_INFO,"ICE-PEER(%s): all checks completed success=%d fail=%d",pctx->label,succeeded,failed);

    


    if (!pctx->reported_done) {
      pctx->reported_done = 1;
      assert(!pctx->done_cb_timer);
      NR_ASYNC_TIMER_SET(0,nr_ice_peer_ctx_fire_done,pctx,&pctx->done_cb_timer);
    }

  done:
    _status=0;

    return(_status);
  }




int nr_ice_peer_ctx_find_component(nr_ice_peer_ctx *pctx, nr_ice_media_stream *str, int component_id, nr_ice_component **compp)
  {
    nr_ice_media_stream *pstr;
    int r,_status;

    pstr=STAILQ_FIRST(&pctx->peer_streams);
    while(pstr){
      if(pstr->local_stream==str)
        break;

      pstr=STAILQ_NEXT(pstr,entry);
    }
    if(!pstr)
      ABORT(R_BAD_ARGS);

    if(r=nr_ice_media_stream_find_component(pstr,component_id,compp))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }













int nr_ice_peer_ctx_deliver_packet_maybe(nr_ice_peer_ctx *pctx, nr_ice_component *comp, nr_transport_addr *source_addr, UCHAR *data, int len)
  {
    nr_ice_component *peer_comp;
    nr_ice_candidate *cand;
    int r,_status;

    if(r=nr_ice_peer_ctx_find_component(pctx, comp->stream, comp->component_id,
      &peer_comp))
      ABORT(r);

    
    cand=TAILQ_FIRST(&peer_comp->candidates);
    while(cand){
      if(!nr_transport_addr_cmp(source_addr,&cand->addr,
        NR_TRANSPORT_ADDR_CMP_MODE_ALL))
        break;

      cand=TAILQ_NEXT(cand,entry_comp);
    }

    if(!cand)
      ABORT(R_REJECTED);

    

    if (pctx->handler) {
      r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s): Delivering data", pctx->label);

      pctx->handler->vtbl->msg_recvd(pctx->handler->obj,
        pctx,comp->stream,comp->component_id,data,len);
    }

    _status=0;
  abort:
    return(_status);
  }


