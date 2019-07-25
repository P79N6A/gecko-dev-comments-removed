

















#include <stdio.h>
#include <ogg/ogg.h>
#include "ivorbiscodec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"
#include "block.h"

static int _vorbis_synthesis1(vorbis_block *vb,ogg_packet *op,int decodep){
  vorbis_dsp_state     *vd= vb ? vb->vd : 0;
  private_state        *b= vd ? (private_state *)vd->backend_state: 0;
  vorbis_info          *vi= vd ? vd->vi : 0;
  codec_setup_info     *ci= vi ? (codec_setup_info *)vi->codec_setup : 0;
  oggpack_buffer       *opb=vb ? &vb->opb : 0;
  int                   type,mode,i;
 
  if (!vd || !b || !vi || !ci || !opb) {
    return OV_EBADPACKET;
  }

  
  _vorbis_block_ripcord(vb);
  oggpack_readinit(opb,op->packet,op->bytes);

  
  if(oggpack_read(opb,1)!=0){
    
    return(OV_ENOTAUDIO);
  }

  
  mode=oggpack_read(opb,b->modebits);
  if(mode==-1)return(OV_EBADPACKET);
  
  vb->mode=mode;
  if(!ci->mode_param[mode]){
    return(OV_EBADPACKET);
  }

  vb->W=ci->mode_param[mode]->blockflag;
  if(vb->W){
    vb->lW=oggpack_read(opb,1);
    vb->nW=oggpack_read(opb,1);
    if(vb->nW==-1)   return(OV_EBADPACKET);
  }else{
    vb->lW=0;
    vb->nW=0;
  }
  
  
  vb->granulepos=op->granulepos;
  vb->sequence=op->packetno-3; 
  vb->eofflag=op->e_o_s;

  if(decodep){
    
    vb->pcmend=ci->blocksizes[vb->W];
    vb->pcm=(ogg_int32_t **)_vorbis_block_alloc(vb,sizeof(*vb->pcm)*vi->channels);
    for(i=0;i<vi->channels;i++)
      vb->pcm[i]=(ogg_int32_t *)_vorbis_block_alloc(vb,vb->pcmend*sizeof(*vb->pcm[i]));
    
    
    type=ci->map_type[ci->mode_param[mode]->mapping];
    
    return(_mapping_P[type]->inverse(vb,b->mode[mode]));
  }else{
    
    vb->pcmend=0;
    vb->pcm=NULL;
    
    return(0);
  }
}

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  return _vorbis_synthesis1(vb,op,1);
}



int vorbis_synthesis_trackonly(vorbis_block *vb,ogg_packet *op){
  return _vorbis_synthesis1(vb,op,0);
}

long vorbis_packet_blocksize(vorbis_info *vi,ogg_packet *op){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  oggpack_buffer       opb;
  int                  mode;
 
  oggpack_readinit(&opb,op->packet,op->bytes);

  
  if(oggpack_read(&opb,1)!=0){
    
    return(OV_ENOTAUDIO);
  }

  {
    int modebits=0;
    int v=ci->modes;
    while(v>1){
      modebits++;
      v>>=1;
    }

    
    mode=oggpack_read(&opb,modebits);
  }
  if(mode==-1)return(OV_EBADPACKET);
  return(ci->blocksizes[ci->mode_param[mode]->blockflag]);
}


