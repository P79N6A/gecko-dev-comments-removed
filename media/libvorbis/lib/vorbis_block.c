



















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "window.h"
#include "mdct.h"
#include "lpc.h"
#include "registry.h"
#include "misc.h"

static int ilog2(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}









































#ifndef WORD_ALIGN
#define WORD_ALIGN 8
#endif

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  int i;
  memset(vb,0,sizeof(*vb));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;
  if(v->analysisp){
    vorbis_block_internal *vbi=
      vb->internal=_ogg_calloc(1,sizeof(vorbis_block_internal));
    vbi->ampmax=-9999;

    for(i=0;i<PACKETBLOBS;i++){
      if(i==PACKETBLOBS/2){
        vbi->packetblob[i]=&vb->opb;
      }else{
        vbi->packetblob[i]=
          _ogg_calloc(1,sizeof(oggpack_buffer));
      }
      oggpack_writeinit(vbi->packetblob[i]);
    }
  }

  return(0);
}

void *_vorbis_block_alloc(vorbis_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    
    if(vb->localstore){
      struct alloc_chain *link=_ogg_malloc(sizeof(*link));
      vb->totaluse+=vb->localtop;
      link->next=vb->reap;
      link->ptr=vb->localstore;
      vb->reap=link;
    }
    
    vb->localalloc=bytes;
    vb->localstore=_ogg_malloc(vb->localalloc);
    vb->localtop=0;
  }
  {
    void *ret=(void *)(((char *)vb->localstore)+vb->localtop);
    vb->localtop+=bytes;
    return ret;
  }
}


void _vorbis_block_ripcord(vorbis_block *vb){
  
  struct alloc_chain *reap=vb->reap;
  while(reap){
    struct alloc_chain *next=reap->next;
    _ogg_free(reap->ptr);
    memset(reap,0,sizeof(*reap));
    _ogg_free(reap);
    reap=next;
  }
  
  if(vb->totaluse){
    vb->localstore=_ogg_realloc(vb->localstore,vb->totaluse+vb->localalloc);
    vb->localalloc+=vb->totaluse;
    vb->totaluse=0;
  }

  
  vb->localtop=0;
  vb->reap=NULL;
}

int vorbis_block_clear(vorbis_block *vb){
  int i;
  vorbis_block_internal *vbi=vb->internal;

  _vorbis_block_ripcord(vb);
  if(vb->localstore)_ogg_free(vb->localstore);

  if(vbi){
    for(i=0;i<PACKETBLOBS;i++){
      oggpack_writeclear(vbi->packetblob[i]);
      if(i!=PACKETBLOBS/2)_ogg_free(vbi->packetblob[i]);
    }
    _ogg_free(vbi);
  }
  memset(vb,0,sizeof(*vb));
  return(0);
}





static int _vds_shared_init(vorbis_dsp_state *v,vorbis_info *vi,int encp){
  int i;
  codec_setup_info *ci=vi->codec_setup;
  private_state *b=NULL;
  int hs;

  if(ci==NULL) return 1;
  hs=ci->halfrate_flag;

  memset(v,0,sizeof(*v));
  b=v->backend_state=_ogg_calloc(1,sizeof(*b));

  v->vi=vi;
  b->modebits=ilog2(ci->modes);

  b->transform[0]=_ogg_calloc(VI_TRANSFORMB,sizeof(*b->transform[0]));
  b->transform[1]=_ogg_calloc(VI_TRANSFORMB,sizeof(*b->transform[1]));

  

  b->transform[0][0]=_ogg_calloc(1,sizeof(mdct_lookup));
  b->transform[1][0]=_ogg_calloc(1,sizeof(mdct_lookup));
  mdct_init(b->transform[0][0],ci->blocksizes[0]>>hs);
  mdct_init(b->transform[1][0],ci->blocksizes[1]>>hs);

  
  b->window[0]=ilog2(ci->blocksizes[0])-6;
  b->window[1]=ilog2(ci->blocksizes[1])-6;

  if(encp){ 

    
    drft_init(&b->fft_look[0],ci->blocksizes[0]);
    drft_init(&b->fft_look[1],ci->blocksizes[1]);

    
    if(!ci->fullbooks){
      ci->fullbooks=_ogg_calloc(ci->books,sizeof(*ci->fullbooks));
      for(i=0;i<ci->books;i++)
        vorbis_book_init_encode(ci->fullbooks+i,ci->book_param[i]);
    }

    b->psy=_ogg_calloc(ci->psys,sizeof(*b->psy));
    for(i=0;i<ci->psys;i++){
      _vp_psy_init(b->psy+i,
                   ci->psy_param[i],
                   &ci->psy_g_param,
                   ci->blocksizes[ci->psy_param[i]->blockflag]/2,
                   vi->rate);
    }

    v->analysisp=1;
  }else{
    
    if(!ci->fullbooks)
      ci->fullbooks=_ogg_calloc(ci->books,sizeof(*ci->fullbooks));
    for(i=0;i<ci->books;i++){
      if(ci->book_param[i]==NULL)
        goto abort_books;
      if(vorbis_book_init_decode(ci->fullbooks+i,ci->book_param[i]))
        goto abort_books;
        
      vorbis_staticbook_destroy(ci->book_param[i]);
      ci->book_param[i]=NULL;
    }
  }

  

  v->pcm_storage=ci->blocksizes[1];
  v->pcm=_ogg_malloc(vi->channels*sizeof(*v->pcm));
  v->pcmret=_ogg_malloc(vi->channels*sizeof(*v->pcmret));
  {
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=_ogg_calloc(v->pcm_storage,sizeof(*v->pcm[i]));
  }

  
  
  v->lW=0; 
  v->W=0;  

  
  v->centerW=ci->blocksizes[1]/2;

  v->pcm_current=v->centerW;

  
  b->flr=_ogg_calloc(ci->floors,sizeof(*b->flr));
  b->residue=_ogg_calloc(ci->residues,sizeof(*b->residue));

  for(i=0;i<ci->floors;i++)
    b->flr[i]=_floor_P[ci->floor_type[i]]->
      look(v,ci->floor_param[i]);

  for(i=0;i<ci->residues;i++)
    b->residue[i]=_residue_P[ci->residue_type[i]]->
      look(v,ci->residue_param[i]);

  return 0;
 abort_books:
  for(i=0;i<ci->books;i++){
    if(ci->book_param[i]!=NULL){
      vorbis_staticbook_destroy(ci->book_param[i]);
      ci->book_param[i]=NULL;
    }
  }
  vorbis_dsp_clear(v);
  return -1;
}


int vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi){
  private_state *b=NULL;

  if(_vds_shared_init(v,vi,1))return 1;
  b=v->backend_state;
  b->psy_g_look=_vp_global_look(vi);

  
  b->ve=_ogg_calloc(1,sizeof(*b->ve));
  _ve_envelope_init(b->ve,vi);

  vorbis_bitrate_init(vi,&b->bms);

  

  v->sequence=3;

  return(0);
}

void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i;
  if(v){
    vorbis_info *vi=v->vi;
    codec_setup_info *ci=(vi?vi->codec_setup:NULL);
    private_state *b=v->backend_state;

    if(b){

      if(b->ve){
        _ve_envelope_clear(b->ve);
        _ogg_free(b->ve);
      }

      if(b->transform[0]){
        mdct_clear(b->transform[0][0]);
        _ogg_free(b->transform[0][0]);
        _ogg_free(b->transform[0]);
      }
      if(b->transform[1]){
        mdct_clear(b->transform[1][0]);
        _ogg_free(b->transform[1][0]);
        _ogg_free(b->transform[1]);
      }

      if(b->flr){
        if(ci)
          for(i=0;i<ci->floors;i++)
            _floor_P[ci->floor_type[i]]->
              free_look(b->flr[i]);
        _ogg_free(b->flr);
      }
      if(b->residue){
        if(ci)
          for(i=0;i<ci->residues;i++)
            _residue_P[ci->residue_type[i]]->
              free_look(b->residue[i]);
        _ogg_free(b->residue);
      }
      if(b->psy){
        if(ci)
          for(i=0;i<ci->psys;i++)
            _vp_psy_clear(b->psy+i);
        _ogg_free(b->psy);
      }

      if(b->psy_g_look)_vp_global_free(b->psy_g_look);
      vorbis_bitrate_clear(&b->bms);

      drft_clear(&b->fft_look[0]);
      drft_clear(&b->fft_look[1]);

    }

    if(v->pcm){
      if(vi)
        for(i=0;i<vi->channels;i++)
          if(v->pcm[i])_ogg_free(v->pcm[i]);
      _ogg_free(v->pcm);
      if(v->pcmret)_ogg_free(v->pcmret);
    }

    if(b){
      
      if(b->header)_ogg_free(b->header);
      if(b->header1)_ogg_free(b->header1);
      if(b->header2)_ogg_free(b->header2);
      _ogg_free(b);
    }

    memset(v,0,sizeof(*v));
  }
}

float **vorbis_analysis_buffer(vorbis_dsp_state *v, int vals){
  int i;
  vorbis_info *vi=v->vi;
  private_state *b=v->backend_state;

  
  if(b->header)_ogg_free(b->header);b->header=NULL;
  if(b->header1)_ogg_free(b->header1);b->header1=NULL;
  if(b->header2)_ogg_free(b->header2);b->header2=NULL;

  


  if(v->pcm_current+vals>=v->pcm_storage){
    v->pcm_storage=v->pcm_current+vals*2;

    for(i=0;i<vi->channels;i++){
      v->pcm[i]=_ogg_realloc(v->pcm[i],v->pcm_storage*sizeof(*v->pcm[i]));
    }
  }

  for(i=0;i<vi->channels;i++)
    v->pcmret[i]=v->pcm[i]+v->pcm_current;

  return(v->pcmret);
}

static void _preextrapolate_helper(vorbis_dsp_state *v){
  int i;
  int order=16;
  float *lpc=alloca(order*sizeof(*lpc));
  float *work=alloca(v->pcm_current*sizeof(*work));
  long j;
  v->preextrapolate=1;

  if(v->pcm_current-v->centerW>order*2){ 
    for(i=0;i<v->vi->channels;i++){
      
      for(j=0;j<v->pcm_current;j++)
        work[j]=v->pcm[i][v->pcm_current-j-1];

      
      vorbis_lpc_from_data(work,lpc,v->pcm_current-v->centerW,order);

#if 0
      if(v->vi->channels==2){
        if(i==0)
          _analysis_output("predataL",0,work,v->pcm_current-v->centerW,0,0,0);
        else
          _analysis_output("predataR",0,work,v->pcm_current-v->centerW,0,0,0);
      }else{
        _analysis_output("predata",0,work,v->pcm_current-v->centerW,0,0,0);
      }
#endif

      
      vorbis_lpc_predict(lpc,work+v->pcm_current-v->centerW-order,
                         order,
                         work+v->pcm_current-v->centerW,
                         v->centerW);

      for(j=0;j<v->pcm_current;j++)
        v->pcm[i][v->pcm_current-j-1]=work[j];

    }
  }
}




int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;

  if(vals<=0){
    int order=32;
    int i;
    float *lpc=alloca(order*sizeof(*lpc));

    
    if(!v->preextrapolate)
      _preextrapolate_helper(v);

    

    



    vorbis_analysis_buffer(v,ci->blocksizes[1]*3);
    v->eofflag=v->pcm_current;
    v->pcm_current+=ci->blocksizes[1]*3;

    for(i=0;i<vi->channels;i++){
      if(v->eofflag>order*2){
        
        long n;

        
        n=v->eofflag;
        if(n>ci->blocksizes[1])n=ci->blocksizes[1];
        vorbis_lpc_from_data(v->pcm[i]+v->eofflag-n,lpc,n,order);

        
        vorbis_lpc_predict(lpc,v->pcm[i]+v->eofflag-order,order,
                           v->pcm[i]+v->eofflag,v->pcm_current-v->eofflag);
      }else{
        


        memset(v->pcm[i]+v->eofflag,0,
               (v->pcm_current-v->eofflag)*sizeof(*v->pcm[i]));

      }
    }
  }else{

    if(v->pcm_current+vals>v->pcm_storage)
      return(OV_EINVAL);

    v->pcm_current+=vals;

    

    
    if(!v->preextrapolate && v->pcm_current-v->centerW>ci->blocksizes[1])
      _preextrapolate_helper(v);

  }
  return(0);
}



int vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb){
  int i;
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  private_state *b=v->backend_state;
  vorbis_look_psy_global *g=b->psy_g_look;
  long beginW=v->centerW-ci->blocksizes[v->W]/2,centerNext;
  vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;

  
  if(!v->preextrapolate)return(0);

  
  if(v->eofflag==-1)return(0);

  



  


  {
    long bp=_ve_envelope_search(v);
    if(bp==-1){

      if(v->eofflag==0)return(0); 

      v->nW=0;
    }else{

      if(ci->blocksizes[0]==ci->blocksizes[1])
        v->nW=0;
      else
        v->nW=bp;
    }
  }

  centerNext=v->centerW+ci->blocksizes[v->W]/4+ci->blocksizes[v->nW]/4;

  {
    

    long blockbound=centerNext+ci->blocksizes[v->nW]/2;
    if(v->pcm_current<blockbound)return(0); 








  }

  


  _vorbis_block_ripcord(vb);
  vb->lW=v->lW;
  vb->W=v->W;
  vb->nW=v->nW;

  if(v->W){
    if(!v->lW || !v->nW){
      vbi->blocktype=BLOCKTYPE_TRANSITION;
      
    }else{
      vbi->blocktype=BLOCKTYPE_LONG;
      
    }
  }else{
    if(_ve_envelope_mark(v)){
      vbi->blocktype=BLOCKTYPE_IMPULSE;
      

    }else{
      vbi->blocktype=BLOCKTYPE_PADDING;
      

    }
  }

  vb->vd=v;
  vb->sequence=v->sequence++;
  vb->granulepos=v->granulepos;
  vb->pcmend=ci->blocksizes[v->W];

  

  
  
  if(vbi->ampmax>g->ampmax)g->ampmax=vbi->ampmax;
  g->ampmax=_vp_ampmax_decay(g->ampmax,v);
  vbi->ampmax=g->ampmax;

  vb->pcm=_vorbis_block_alloc(vb,sizeof(*vb->pcm)*vi->channels);
  vbi->pcmdelay=_vorbis_block_alloc(vb,sizeof(*vbi->pcmdelay)*vi->channels);
  for(i=0;i<vi->channels;i++){
    vbi->pcmdelay[i]=
      _vorbis_block_alloc(vb,(vb->pcmend+beginW)*sizeof(*vbi->pcmdelay[i]));
    memcpy(vbi->pcmdelay[i],v->pcm[i],(vb->pcmend+beginW)*sizeof(*vbi->pcmdelay[i]));
    vb->pcm[i]=vbi->pcmdelay[i]+beginW;

    




  }

  



  if(v->eofflag){
    if(v->centerW>=v->eofflag){
      v->eofflag=-1;
      vb->eofflag=1;
      return(1);
    }
  }

  
  {
    int new_centerNext=ci->blocksizes[1]/2;
    int movementW=centerNext-new_centerNext;

    if(movementW>0){

      _ve_envelope_shift(b->ve,movementW);
      v->pcm_current-=movementW;

      for(i=0;i<vi->channels;i++)
        memmove(v->pcm[i],v->pcm[i]+movementW,
                v->pcm_current*sizeof(*v->pcm[i]));


      v->lW=v->W;
      v->W=v->nW;
      v->centerW=new_centerNext;

      if(v->eofflag){
        v->eofflag-=movementW;
        if(v->eofflag<=0)v->eofflag=-1;
        
        if(v->centerW>=v->eofflag){
          v->granulepos+=movementW-(v->centerW-v->eofflag);
        }else{
          v->granulepos+=movementW;
        }
      }else{
        v->granulepos+=movementW;
      }
    }
  }

  
  return(1);
}

int vorbis_synthesis_restart(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci;
  int hs;

  if(!v->backend_state)return -1;
  if(!vi)return -1;
  ci=vi->codec_setup;
  if(!ci)return -1;
  hs=ci->halfrate_flag;

  v->centerW=ci->blocksizes[1]>>(hs+1);
  v->pcm_current=v->centerW>>hs;

  v->pcm_returned=-1;
  v->granulepos=-1;
  v->sequence=-1;
  v->eofflag=0;
  ((private_state *)(v->backend_state))->sample_count=-1;

  return(0);
}

int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  if(_vds_shared_init(v,vi,0)){
    vorbis_dsp_clear(v);
    return 1;
  }
  vorbis_synthesis_restart(v);
  return 0;
}





int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  private_state *b=v->backend_state;
  int hs=ci->halfrate_flag;
  int i,j;

  if(!vb)return(OV_EINVAL);
  if(v->pcm_current>v->pcm_returned  && v->pcm_returned!=-1)return(OV_EINVAL);

  v->lW=v->W;
  v->W=vb->W;
  v->nW=-1;

  if((v->sequence==-1)||
     (v->sequence+1 != vb->sequence)){
    v->granulepos=-1; 
    b->sample_count=-1;
  }

  v->sequence=vb->sequence;

  if(vb->pcm){  

    int n=ci->blocksizes[v->W]>>(hs+1);
    int n0=ci->blocksizes[0]>>(hs+1);
    int n1=ci->blocksizes[1]>>(hs+1);

    int thisCenter;
    int prevCenter;

    v->glue_bits+=vb->glue_bits;
    v->time_bits+=vb->time_bits;
    v->floor_bits+=vb->floor_bits;
    v->res_bits+=vb->res_bits;

    if(v->centerW){
      thisCenter=n1;
      prevCenter=0;
    }else{
      thisCenter=0;
      prevCenter=n1;
    }

    



    for(j=0;j<vi->channels;j++){
      
      if(v->lW){
        if(v->W){
          
          float *w=_vorbis_window_get(b->window[1]-hs);
          float *pcm=v->pcm[j]+prevCenter;
          float *p=vb->pcm[j];
          for(i=0;i<n1;i++)
            pcm[i]=pcm[i]*w[n1-i-1] + p[i]*w[i];
        }else{
          
          float *w=_vorbis_window_get(b->window[0]-hs);
          float *pcm=v->pcm[j]+prevCenter+n1/2-n0/2;
          float *p=vb->pcm[j];
          for(i=0;i<n0;i++)
            pcm[i]=pcm[i]*w[n0-i-1] +p[i]*w[i];
        }
      }else{
        if(v->W){
          
          float *w=_vorbis_window_get(b->window[0]-hs);
          float *pcm=v->pcm[j]+prevCenter;
          float *p=vb->pcm[j]+n1/2-n0/2;
          for(i=0;i<n0;i++)
            pcm[i]=pcm[i]*w[n0-i-1] +p[i]*w[i];
          for(;i<n1/2+n0/2;i++)
            pcm[i]=p[i];
        }else{
          
          float *w=_vorbis_window_get(b->window[0]-hs);
          float *pcm=v->pcm[j]+prevCenter;
          float *p=vb->pcm[j];
          for(i=0;i<n0;i++)
            pcm[i]=pcm[i]*w[n0-i-1] +p[i]*w[i];
        }
      }

      
      {
        float *pcm=v->pcm[j]+thisCenter;
        float *p=vb->pcm[j]+n;
        for(i=0;i<n;i++)
          pcm[i]=p[i];
      }
    }

    if(v->centerW)
      v->centerW=0;
    else
      v->centerW=n1;

    



    if(v->pcm_returned==-1){
      v->pcm_returned=thisCenter;
      v->pcm_current=thisCenter;
    }else{
      v->pcm_returned=prevCenter;
      v->pcm_current=prevCenter+
        ((ci->blocksizes[v->lW]/4+
        ci->blocksizes[v->W]/4)>>hs);
    }

  }

  










  if(b->sample_count==-1){
    b->sample_count=0;
  }else{
    b->sample_count+=ci->blocksizes[v->lW]/4+ci->blocksizes[v->W]/4;
  }

  if(v->granulepos==-1){
    if(vb->granulepos!=-1){ 

      v->granulepos=vb->granulepos;

      
      if(b->sample_count>v->granulepos){
        

        if(vb->eofflag){
          
          

          


          v->pcm_current-=(b->sample_count-v->granulepos)>>hs;
        }else{
          
          v->pcm_returned+=(b->sample_count-v->granulepos)>>hs;
          if(v->pcm_returned>v->pcm_current)
            v->pcm_returned=v->pcm_current;
        }

      }

    }
  }else{
    v->granulepos+=ci->blocksizes[v->lW]/4+ci->blocksizes[v->W]/4;
    if(vb->granulepos!=-1 && v->granulepos!=vb->granulepos){

      if(v->granulepos>vb->granulepos){
        long extra=v->granulepos-vb->granulepos;

        if(extra)
          if(vb->eofflag){
            
            v->pcm_current-=extra>>hs;
          } 

      } 

      v->granulepos=vb->granulepos;
    }
  }

  

  if(vb->eofflag)v->eofflag=1;
  return(0);

}


int vorbis_synthesis_pcmout(vorbis_dsp_state *v,float ***pcm){
  vorbis_info *vi=v->vi;

  if(v->pcm_returned>-1 && v->pcm_returned<v->pcm_current){
    if(pcm){
      int i;
      for(i=0;i<vi->channels;i++)
        v->pcmret[i]=v->pcm[i]+v->pcm_returned;
      *pcm=v->pcmret;
    }
    return(v->pcm_current-v->pcm_returned);
  }
  return(0);
}

int vorbis_synthesis_read(vorbis_dsp_state *v,int n){
  if(n && v->pcm_returned+n>v->pcm_current)return(OV_EINVAL);
  v->pcm_returned+=n;
  return(0);
}






int vorbis_synthesis_lapout(vorbis_dsp_state *v,float ***pcm){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  int hs=ci->halfrate_flag;

  int n=ci->blocksizes[v->W]>>(hs+1);
  int n0=ci->blocksizes[0]>>(hs+1);
  int n1=ci->blocksizes[1]>>(hs+1);
  int i,j;

  if(v->pcm_returned<0)return 0;

  







  

  if(v->centerW==n1){
    
    
    for(j=0;j<vi->channels;j++){
      float *p=v->pcm[j];
      for(i=0;i<n1;i++){
        float temp=p[i];
        p[i]=p[i+n1];
        p[i+n1]=temp;
      }
    }

    v->pcm_current-=n1;
    v->pcm_returned-=n1;
    v->centerW=0;
  }

  
  if((v->lW^v->W)==1){
    
    for(j=0;j<vi->channels;j++){
      float *s=v->pcm[j];
      float *d=v->pcm[j]+(n1-n0)/2;
      for(i=(n1+n0)/2-1;i>=0;--i)
        d[i]=s[i];
    }
    v->pcm_returned+=(n1-n0)/2;
    v->pcm_current+=(n1-n0)/2;
  }else{
    if(v->lW==0){
      
      for(j=0;j<vi->channels;j++){
        float *s=v->pcm[j];
        float *d=v->pcm[j]+n1-n0;
        for(i=n0-1;i>=0;--i)
          d[i]=s[i];
      }
      v->pcm_returned+=n1-n0;
      v->pcm_current+=n1-n0;
    }
  }

  if(pcm){
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcmret[i]=v->pcm[i]+v->pcm_returned;
    *pcm=v->pcmret;
  }

  return(n1+n-v->pcm_returned);

}

float *vorbis_window(vorbis_dsp_state *v,int W){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  int hs=ci->halfrate_flag;
  private_state *b=v->backend_state;

  if(b->window[W]-1<0)return NULL;
  return _vorbis_window_get(b->window[W]-hs);
}
