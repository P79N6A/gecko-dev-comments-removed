

































static char *RCSSTRING __UNUSED__="$Id: ice_candidate_pair.c,v 1.2 2008/04/28 17:59:01 ekr Exp $";

#include <assert.h>
#include <string.h>
#include <nr_api.h>
#include "async_timer.h"
#include "ice_ctx.h"
#include "ice_util.h"
#include "ice_codeword.h"
#include "stun.h"

static char *nr_ice_cand_pair_states[]={"UNKNOWN","FROZEN","WAITING","IN_PROGRESS","FAILED","SUCCEEDED","CANCELLED"};

static void nr_ice_candidate_pair_restart_stun_controlled_cb(NR_SOCKET s, int how, void *cb_arg);
static void nr_ice_candidate_pair_compute_codeword(nr_ice_cand_pair *pair,
  nr_ice_candidate *lcand, nr_ice_candidate *rcand);

int nr_ice_candidate_pair_create(nr_ice_peer_ctx *pctx, nr_ice_candidate *lcand,nr_ice_candidate *rcand,nr_ice_cand_pair **pairp)
  {
    nr_ice_cand_pair *pair=0;
    UINT8 o_priority, a_priority;
    char *lufrag,*rufrag;
    char *lpwd,*rpwd;
    char *l2ruser=0,*r2lpass=0;
    int r,_status;
    UINT4 RTO;
    nr_ice_candidate tmpcand;
    UINT8 t_priority;

    if(!(pair=RCALLOC(sizeof(nr_ice_cand_pair))))
      ABORT(R_NO_MEMORY);

    pair->pctx=pctx;

    nr_ice_candidate_pair_compute_codeword(pair,lcand,rcand);

    if(r=nr_concat_strings(&pair->as_string,pair->codeword,"|",lcand->addr.as_string,"|",
        rcand->addr.as_string,"(",lcand->label,"|",rcand->label,")", NULL))
      ABORT(r);

    nr_ice_candidate_pair_set_state(pctx,pair,NR_ICE_PAIR_STATE_FROZEN);
    pair->local=lcand;
    pair->remote=rcand;

    
    if(pctx->ctx->flags & NR_ICE_CTX_FLAGS_OFFERER)
    {
      assert(!(pctx->ctx->flags & NR_ICE_CTX_FLAGS_ANSWERER));

      o_priority=lcand->priority;
      a_priority=rcand->priority;
    }
    else{
      o_priority=rcand->priority;
      a_priority=lcand->priority;
    }
    pair->priority=(MIN(o_priority, a_priority))<<32 |
      (MAX(o_priority, a_priority))<<1 | (o_priority > a_priority?0:1);

    r_log(LOG_ICE,LOG_DEBUG,"ICE(%s): Pairing candidate %s (%x):%s (%x) priority=%llu (%llx) codeword=%s",pctx->ctx->label,lcand->addr.as_string,lcand->priority,rcand->addr.as_string,rcand->priority,pair->priority,pair->priority,pair->codeword);

    
    if(r=nr_concat_strings(&pair->foundation,lcand->foundation,"|",
      rcand->foundation,NULL))
      ABORT(r);


    
    lufrag=lcand->stream->ufrag?lcand->stream->ufrag:pctx->ctx->ufrag;
    lpwd=lcand->stream->pwd?lcand->stream->pwd:pctx->ctx->pwd;
    assert(lufrag);
    assert(lpwd);
    rufrag=rcand->stream->ufrag?rcand->stream->ufrag:pctx->peer_ufrag;
    rpwd=rcand->stream->pwd?rcand->stream->pwd:pctx->peer_pwd;
    if (!rufrag || !rpwd)
      ABORT(R_BAD_DATA);

    
    RTO = MAX(100, (pctx->ctx->Ta * pctx->waiting_pairs));

    

    memcpy(&tmpcand, lcand, sizeof(tmpcand));
    tmpcand.type = PEER_REFLEXIVE;
    if (r=nr_ice_candidate_compute_priority(&tmpcand))
      ABORT(r);
    t_priority = tmpcand.priority;

    
    if(r=nr_concat_strings(&l2ruser,rufrag,":",lufrag,NULL))
      ABORT(r);
    if(r=nr_stun_client_ctx_create(pair->as_string,
      lcand->osock,
      &rcand->addr,RTO,&pair->stun_client))
      ABORT(r);
    if(!(pair->stun_client->params.ice_binding_request.username=r_strdup(l2ruser)))
      ABORT(R_NO_MEMORY);
    if(r=r_data_make(&pair->stun_client->params.ice_binding_request.password,(UCHAR *)rpwd,strlen(rpwd)))
      ABORT(r);
    pair->stun_client->params.ice_binding_request.priority=t_priority;
    pair->stun_client->params.ice_binding_request.control = pctx->controlling?
      NR_ICE_CONTROLLING:NR_ICE_CONTROLLED;

    pair->stun_client->params.ice_binding_request.tiebreaker=pctx->tiebreaker;

    

    if(r=nr_concat_strings(&pair->r2l_user,lufrag,":",rufrag,NULL))
      ABORT(r);
    if(!(r2lpass=r_strdup(lpwd)))
      ABORT(R_NO_MEMORY);
    INIT_DATA(pair->r2l_pwd,(UCHAR *)r2lpass,strlen(r2lpass));
    
    r2lpass=0;

    *pairp=pair;

    _status=0;
  abort:
    RFREE(l2ruser);
    RFREE(r2lpass);
    if(_status){
      nr_ice_candidate_pair_destroy(&pair);
    }
    return(_status);
  }

int nr_ice_candidate_pair_destroy(nr_ice_cand_pair **pairp)
  {
    nr_ice_cand_pair *pair;

    if(!pairp || !*pairp)
      return(0);

    pair=*pairp;
    *pairp=0;

    RFREE(pair->as_string);
    RFREE(pair->foundation);
    nr_ice_socket_deregister(pair->local->isock,pair->stun_client_handle);
    if (pair->stun_client) {
      RFREE(pair->stun_client->params.ice_binding_request.username);
      RFREE(pair->stun_client->params.ice_binding_request.password.data);
      nr_stun_client_ctx_destroy(&pair->stun_client);
    }

    RFREE(pair->r2l_user);
    RFREE(pair->r2l_pwd.data);

    NR_async_timer_cancel(pair->stun_cb_timer);
    NR_async_timer_cancel(pair->restart_controlled_cb_timer);
    NR_async_timer_cancel(pair->restart_nominated_cb_timer);

    RFREE(pair);
    return(0);
  }

int nr_ice_candidate_pair_unfreeze(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair)
  {
    assert(pair->state==NR_ICE_PAIR_STATE_FROZEN);

    nr_ice_candidate_pair_set_state(pctx,pair,NR_ICE_PAIR_STATE_WAITING);

    return(0);
  }

static void nr_ice_candidate_pair_stun_cb(NR_SOCKET s, int how, void *cb_arg)
  {
    int r,_status;
    nr_ice_cand_pair *pair=cb_arg,*orig_pair;
    nr_ice_candidate *cand=0;
    nr_stun_message *sres;
    nr_transport_addr *request_src;
    nr_transport_addr *request_dst;
    nr_transport_addr *response_src;
    nr_transport_addr response_dst;
    nr_stun_message_attribute *attr;

    pair->stun_cb_timer=0;

    r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s)/STREAM(%s): STUN cb on pair %s",
      pair->pctx->label,pair->local->stream->label,pair->as_string);

    


    if(pair->state==NR_ICE_PAIR_STATE_SUCCEEDED)
      goto done;

    switch(pair->stun_client->state){
      case NR_STUN_CLIENT_STATE_FAILED:
        sres=pair->stun_client->response;
        if(sres && nr_stun_message_has_attribute(sres,NR_STUN_ATTR_ERROR_CODE,&attr)&&attr->u.error_code.number==487){
          r_log(LOG_ICE,LOG_ERR,"ICE-PEER(%s): detected role conflict. Switching to controlled",pair->pctx->label);

          pair->pctx->controlling=0;

          
          if(!pair->restart_controlled_cb_timer)
            NR_ASYNC_TIMER_SET(0,nr_ice_candidate_pair_restart_stun_controlled_cb,pair,&pair->restart_controlled_cb_timer);

          return;
        }
        
      case NR_STUN_CLIENT_STATE_TIMED_OUT:
        nr_ice_candidate_pair_set_state(pair->pctx,pair,NR_ICE_PAIR_STATE_FAILED);
        break;
      case NR_STUN_CLIENT_STATE_DONE:
        
        response_src=&pair->stun_client->peer_addr;
        request_dst=&pair->remote->addr;
        if (nr_transport_addr_cmp(response_src,request_dst,NR_TRANSPORT_ADDR_CMP_MODE_ALL)){
          r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s): Peer address mismatch %s/%s",pair->pctx->label, response_src->as_string,request_dst->as_string);
          nr_ice_candidate_pair_set_state(pair->pctx,pair,NR_ICE_PAIR_STATE_FAILED);
          break;
        }
        request_src=&pair->stun_client->my_addr;
        nr_socket_getaddr(pair->local->isock->sock,&response_dst);
        if (nr_transport_addr_cmp(request_src,&response_dst,NR_TRANSPORT_ADDR_CMP_MODE_ALL)){
          r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s): Address mismatch %s/%s",pair->pctx->label, request_src->as_string,response_dst.as_string);
          nr_ice_candidate_pair_set_state(pair->pctx,pair,NR_ICE_PAIR_STATE_FAILED);
          break;
        }

        if(strlen(pair->stun_client->results.ice_binding_response.mapped_addr.as_string)==0){
          






          r_log(LOG_ICE,LOG_ERR,"ICE-PEER(%s): server failed to return mapped address on pair %s", pair->pctx->label,pair->as_string);
          nr_ice_candidate_pair_set_state(pair->pctx,pair,NR_ICE_PAIR_STATE_FAILED);
          break;
        }
        else if(!nr_transport_addr_cmp(&pair->local->addr,&pair->stun_client->results.ice_binding_response.mapped_addr,NR_TRANSPORT_ADDR_CMP_MODE_ALL)){
          nr_ice_candidate_pair_set_state(pair->pctx,pair,NR_ICE_PAIR_STATE_SUCCEEDED);
        }
        else{
          


          cand=TAILQ_FIRST(&pair->local->component->candidates);
          while(cand){
            if(!nr_transport_addr_cmp(&cand->addr,&pair->stun_client->results.ice_binding_response.mapped_addr,NR_TRANSPORT_ADDR_CMP_MODE_ALL))
              break;

            cand=TAILQ_NEXT(cand,entry_comp);
          }

          
          if(!cand){
            if(r=nr_ice_candidate_create(pair->pctx->ctx,
              pair->local->component,pair->local->isock,pair->local->osock,
              PEER_REFLEXIVE,0,pair->local->component->component_id,&cand))
              ABORT(r);
            if(r=nr_transport_addr_copy(&cand->addr,&pair->stun_client->results.ice_binding_response.mapped_addr))
              ABORT(r);
            cand->state=NR_ICE_CAND_STATE_INITIALIZED;
            TAILQ_INSERT_TAIL(&pair->local->component->candidates,cand,entry_comp);
          }

          
          orig_pair=pair;
          if(r=nr_ice_candidate_pair_create(pair->pctx,cand,pair->remote,
            &pair))
            ABORT(r);

          nr_ice_candidate_pair_set_state(pair->pctx,pair,NR_ICE_PAIR_STATE_SUCCEEDED);

          if(r=nr_ice_candidate_pair_insert(&pair->remote->stream->check_list,pair))
            ABORT(r);

          

          if(orig_pair->peer_nominated)
            pair->peer_nominated=1;


          
          nr_ice_candidate_pair_set_state(orig_pair->pctx,orig_pair,NR_ICE_PAIR_STATE_FAILED);

        }

        
        if(pair->pctx->controlling){
          if(pair->pctx->ctx->flags & NR_ICE_CTX_FLAGS_AGGRESSIVE_NOMINATION)
            pair->nominated=1;
        }
        else{
          if(pair->peer_nominated)
            pair->nominated=1;
        }


        
        
        pair->remote->component->valid_pairs++;

        
        if(r=nr_ice_media_stream_unfreeze_pairs_foundation(pair->remote->stream,pair->foundation))
          ABORT(r);

        
        if(pair->nominated){
          if(r=nr_ice_component_nominated_pair(pair->remote->component, pair))
            ABORT(r);
        }

        break;
      default:
        ABORT(R_INTERNAL);
    }

    

    if(pair->pctx->controlling && !(pair->pctx->ctx->flags & NR_ICE_CTX_FLAGS_AGGRESSIVE_NOMINATION)){

      if(r=nr_ice_component_select_pair(pair->pctx,pair->remote->component)){
        if(r!=R_NOT_FOUND)
          ABORT(r);
      }
    }

  done:
    _status=0;
  abort:
    return;
  }

int nr_ice_candidate_pair_start(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair)
  {
    int r,_status;
    UINT4 mode;

    nr_ice_candidate_pair_set_state(pctx,pair,NR_ICE_PAIR_STATE_IN_PROGRESS);

    
    if(r=nr_ice_socket_register_stun_client(pair->local->isock,pair->stun_client,&pair->stun_client_handle))
      ABORT(r);

    
    if(pair->pctx->controlling && (pair->pctx->ctx->flags & NR_ICE_CTX_FLAGS_AGGRESSIVE_NOMINATION))
      mode=NR_ICE_CLIENT_MODE_USE_CANDIDATE;
    else
      mode=NR_ICE_CLIENT_MODE_BINDING_REQUEST;

    if(r=nr_stun_client_start(pair->stun_client,mode,nr_ice_candidate_pair_stun_cb,pair))
      ABORT(r);

    if ((r=nr_ice_ctx_remember_id(pair->pctx->ctx, pair->stun_client->request))) {
      


      assert(0);
    }

    _status=0;
  abort:
    if(_status){
      
      assert(!pair->stun_cb_timer);
      NR_ASYNC_TIMER_SET(0,nr_ice_candidate_pair_stun_cb,pair, &pair->stun_cb_timer);
      _status=0;
    }
    return(_status);
  }


int nr_ice_candidate_pair_do_triggered_check(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair)
  {
    int r,_status;

    r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s): triggered check on %s",pctx->label,pair->as_string);

    switch(pair->state){
      case NR_ICE_PAIR_STATE_FROZEN:
        nr_ice_candidate_pair_set_state(pctx,pair,NR_ICE_PAIR_STATE_WAITING);
        
      case NR_ICE_PAIR_STATE_WAITING:
        
        if(r=nr_ice_candidate_pair_start(pctx,pair))
          ABORT(r);
        break;
      case NR_ICE_PAIR_STATE_IN_PROGRESS:
        if(r=nr_stun_client_force_retransmit(pair->stun_client))
          ABORT(r);
        break;
      default:
        break;
    }

    
    if(pair->remote->stream->ice_state==NR_ICE_MEDIA_STREAM_CHECKS_FROZEN){
      if(r=nr_ice_media_stream_start_checks(pair->pctx,pair->remote->stream))
        ABORT(r);
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_ice_candidate_pair_cancel(nr_ice_peer_ctx *pctx,nr_ice_cand_pair *pair)
  {
    if(pair->state != NR_ICE_PAIR_STATE_FAILED){
      
      if(pair->state==NR_ICE_PAIR_STATE_IN_PROGRESS){
        nr_stun_client_cancel(pair->stun_client);
      }
      nr_ice_candidate_pair_set_state(pctx,pair,NR_ICE_PAIR_STATE_CANCELLED);
    }

    return(0);
  }

int nr_ice_candidate_pair_select(nr_ice_cand_pair *pair)
  {
    int r,_status;

    if(!pair){
      r_log(LOG_ICE,LOG_ERR,"ICE-PAIR: No pair chosen");
      ABORT(R_BAD_ARGS);
    }

    if(pair->state!=NR_ICE_PAIR_STATE_SUCCEEDED){
      r_log(LOG_ICE,LOG_ERR,"ICE-PEER(%s): tried to install non-succeeded pair %s, ignoring",pair->pctx->label,pair->as_string);
    }
    else{
      
      



      if(!pair->restart_nominated_cb_timer)
        NR_ASYNC_TIMER_SET(0,nr_ice_candidate_pair_restart_stun_nominated_cb,pair,&pair->restart_nominated_cb_timer);

      
      if(r=nr_ice_component_nominated_pair(pair->remote->component, pair))
        ABORT(r);
    }

    _status=0;
  abort:
    return(_status);
 }

int nr_ice_candidate_pair_set_state(nr_ice_peer_ctx *pctx, nr_ice_cand_pair *pair, int state)
  {
    int r,_status;

    r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s): setting pair %s to %s",
      pctx->label,pair->as_string,nr_ice_cand_pair_states[state]);

    












    if(pair->state!=NR_ICE_PAIR_STATE_WAITING){
      if(state==NR_ICE_PAIR_STATE_WAITING)
        pctx->waiting_pairs++;
    }
    else{
      if(state!=NR_ICE_PAIR_STATE_WAITING)
        pctx->waiting_pairs--;

      assert(pctx->waiting_pairs>=0);
    }
    pair->state=state;


    if(pair->state==NR_ICE_PAIR_STATE_FAILED){
      if(r=nr_ice_component_failed_pair(pair->remote->component, pair))
        ABORT(r);
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_ice_candidate_pair_dump_state(nr_ice_cand_pair *pair, FILE *out)
  {
    

    return(0);
  }


int nr_ice_candidate_pair_insert(nr_ice_cand_pair_head *head,nr_ice_cand_pair *pair)
  {
    nr_ice_cand_pair *c1;

    c1=TAILQ_FIRST(head);
    while(c1){
      if(c1->priority < pair->priority){
        TAILQ_INSERT_BEFORE(c1,pair,entry);
        break;
      }

      c1=TAILQ_NEXT(c1,entry);
    }
    if(!c1) TAILQ_INSERT_TAIL(head,pair,entry);

    return(0);
  }

void nr_ice_candidate_pair_restart_stun_nominated_cb(NR_SOCKET s, int how, void *cb_arg)
  {
    nr_ice_cand_pair *pair=cb_arg;
    int r,_status;

    pair->restart_nominated_cb_timer=0;

    r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s)/STREAM(%s):%d: Restarting pair %s as nominated",pair->pctx->label,pair->local->stream->label,pair->remote->component->component_id,pair->as_string);

    nr_stun_client_reset(pair->stun_client);
    pair->stun_client->params.ice_binding_request.control=NR_ICE_CONTROLLING;

    if(r=nr_stun_client_start(pair->stun_client,NR_ICE_CLIENT_MODE_USE_CANDIDATE,nr_ice_candidate_pair_stun_cb,pair))
      ABORT(r);

    if(r=nr_ice_ctx_remember_id(pair->pctx->ctx, pair->stun_client->request))
      ABORT(r);

    _status=0;
  abort:
    return;
  }

static void nr_ice_candidate_pair_restart_stun_controlled_cb(NR_SOCKET s, int how, void *cb_arg)
  {
    nr_ice_cand_pair *pair=cb_arg;
    int r,_status;

    pair->restart_controlled_cb_timer=0;

    r_log(LOG_ICE,LOG_DEBUG,"ICE-PEER(%s)/STREAM(%s):%d: Restarting pair %s as CONTROLLED",pair->pctx->label,pair->local->stream->label,pair->remote->component->component_id,pair->as_string);

    nr_stun_client_reset(pair->stun_client);
    pair->stun_client->params.ice_binding_request.control=NR_ICE_CONTROLLED;

    if(r=nr_stun_client_start(pair->stun_client,NR_ICE_CLIENT_MODE_BINDING_REQUEST,nr_ice_candidate_pair_stun_cb,pair))
      ABORT(r);

    if(r=nr_ice_ctx_remember_id(pair->pctx->ctx, pair->stun_client->request))
      ABORT(r);

    _status=0;
  abort:
    return;
  }



static void nr_ice_candidate_pair_compute_codeword(nr_ice_cand_pair *pair,
  nr_ice_candidate *lcand, nr_ice_candidate *rcand)
  {
    int r,_status;
    char *as_string=0;

    if(r=nr_concat_strings(&as_string,lcand->addr.as_string,"|",
      rcand->addr.as_string,"(",lcand->label,"|",rcand->label,")",NULL))
      ABORT(r);

    nr_ice_compute_codeword(as_string,strlen(as_string),pair->codeword);

    _status=0;
      abort:
    RFREE(as_string);
return;
  }

