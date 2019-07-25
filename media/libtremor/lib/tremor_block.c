
















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "ivorbiscodec.h"
#include "codec_internal.h"

#include "window.h"
#include "registry.h"
#include "misc.h"

static int ilog(unsigned int v){
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
  memset(vb,0,sizeof(*vb));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;
  
  return(0);
}

void *_vorbis_block_alloc(vorbis_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    
    if(vb->localstore){
      struct alloc_chain *link=(struct alloc_chain *)_ogg_malloc(sizeof(*link));
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
  _vorbis_block_ripcord(vb);
  if(vb->localstore)_ogg_free(vb->localstore);

  memset(vb,0,sizeof(*vb));
  return(0);
}

static int _vds_init(vorbis_dsp_state *v,vorbis_info *vi){
  int i;
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;
  private_state *b=NULL;

  if(ci==NULL) return 1;

  memset(v,0,sizeof(*v));
  b=(private_state *)(v->backend_state=_ogg_calloc(1,sizeof(*b)));

  v->vi=vi;
  b->modebits=ilog(ci->modes);

  
  b->window[0]=_vorbis_window(0,ci->blocksizes[0]/2);
  b->window[1]=_vorbis_window(0,ci->blocksizes[1]/2);

  
  if(!ci->fullbooks){
    ci->fullbooks=(codebook *)_ogg_calloc(ci->books,sizeof(*ci->fullbooks));
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
  v->pcm=(ogg_int32_t **)_ogg_malloc(vi->channels*sizeof(*v->pcm));
  v->pcmret=(ogg_int32_t **)_ogg_malloc(vi->channels*sizeof(*v->pcmret));
  for(i=0;i<vi->channels;i++)
    v->pcm[i]=(ogg_int32_t *)_ogg_calloc(v->pcm_storage,sizeof(*v->pcm[i]));

  
  
  v->lW=0; 
  v->W=0;  

  
  b->mode=(vorbis_look_mapping **)_ogg_calloc(ci->modes,sizeof(*b->mode));
  for(i=0;i<ci->modes;i++){
    int mapnum=ci->mode_param[i]->mapping;
    int maptype=ci->map_type[mapnum];
    b->mode[i]=_mapping_P[maptype]->look(v,ci->mode_param[i],
					 ci->map_param[mapnum]);
  }
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

int vorbis_synthesis_restart(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci;

  if(!v->backend_state)return -1;
  if(!vi)return -1;
  ci=vi->codec_setup;
  if(!ci)return -1;

  v->centerW=ci->blocksizes[1]/2;
  v->pcm_current=v->centerW;
  
  v->pcm_returned=-1;
  v->granulepos=-1;
  v->sequence=-1;
  ((private_state *)(v->backend_state))->sample_count=-1;

  return(0);
}

int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  if(_vds_init(v,vi))return 1;
  vorbis_synthesis_restart(v);

  return 0;
}

void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i;
  if(v){
    vorbis_info *vi=v->vi;
    codec_setup_info *ci=(codec_setup_info *)(vi?vi->codec_setup:NULL);
    private_state *b=(private_state *)v->backend_state;

    if(v->pcm){
      for(i=0;i<vi->channels;i++)
	if(v->pcm[i])_ogg_free(v->pcm[i]);
      _ogg_free(v->pcm);
      if(v->pcmret)_ogg_free(v->pcmret);
    }

    
    if(ci){
      for(i=0;i<ci->modes;i++){
	int mapnum=ci->mode_param[i]->mapping;
	int maptype=ci->map_type[mapnum];
	if(b && b->mode)_mapping_P[maptype]->free_look(b->mode[i]);
      }
    }

    if(b){
      if(b->mode)_ogg_free(b->mode);    
      _ogg_free(b);
    }
    
    memset(v,0,sizeof(*v));
  }
}





int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;
  private_state *b=v->backend_state;
  int i,j;

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

    int n=ci->blocksizes[v->W]/2;
    int n0=ci->blocksizes[0]/2;
    int n1=ci->blocksizes[1]/2;
    
    int thisCenter;
    int prevCenter;
    
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
	  
	  ogg_int32_t *pcm=v->pcm[j]+prevCenter;
	  ogg_int32_t *p=vb->pcm[j];
	  for(i=0;i<n1;i++)
	    pcm[i]+=p[i];
	}else{
	  
	  ogg_int32_t *pcm=v->pcm[j]+prevCenter+n1/2-n0/2;
	  ogg_int32_t *p=vb->pcm[j];
	  for(i=0;i<n0;i++)
	    pcm[i]+=p[i];
	}
      }else{
	if(v->W){
	  
	  ogg_int32_t *pcm=v->pcm[j]+prevCenter;
	  ogg_int32_t *p=vb->pcm[j]+n1/2-n0/2;
	  for(i=0;i<n0;i++)
	    pcm[i]+=p[i];
	  for(;i<n1/2+n0/2;i++)
	    pcm[i]=p[i];
	}else{
	  
	  ogg_int32_t *pcm=v->pcm[j]+prevCenter;
	  ogg_int32_t *p=vb->pcm[j];
	  for(i=0;i<n0;i++)
	    pcm[i]+=p[i];
	}
      }
      
      
      {
	ogg_int32_t *pcm=v->pcm[j]+thisCenter;
	ogg_int32_t *p=vb->pcm[j]+n;
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
	ci->blocksizes[v->lW]/4+
	ci->blocksizes[v->W]/4;
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
	

	long extra=b->sample_count-vb->granulepos;

        



        if(extra<0)
          extra=0;

	if(vb->eofflag){
	  
	  

	  


          


          if(extra > v->pcm_current - v->pcm_returned)
            extra = v->pcm_current - v->pcm_returned;

	  v->pcm_current-=extra;
	}else{
	  
	  v->pcm_returned+=extra;
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
	    

            


            if(extra > v->pcm_current - v->pcm_returned)
              extra = v->pcm_current - v->pcm_returned;

            



            if(extra<0)
              extra=0;

            v->pcm_current-=extra;

	  } 

      } 

      v->granulepos=vb->granulepos;
    }
  }
  
  
  
  if(vb->eofflag)v->eofflag=1;
  return(0);
}


int vorbis_synthesis_pcmout(vorbis_dsp_state *v,ogg_int32_t ***pcm){
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

int vorbis_synthesis_read(vorbis_dsp_state *v,int bytes){
  if(bytes && v->pcm_returned+bytes>v->pcm_current)return(OV_EINVAL);
  v->pcm_returned+=bytes;
  return(0);
}

