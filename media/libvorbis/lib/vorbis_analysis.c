
















#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "scales.h"
#include "os.h"
#include "misc.h"

int analysis_noisy=1;


int vorbis_analysis(vorbis_block *vb, ogg_packet *op){
  int ret,i;
  vorbis_block_internal *vbi=vb->internal;

  vb->glue_bits=0;
  vb->time_bits=0;
  vb->floor_bits=0;
  vb->res_bits=0;

  
  for(i=0;i<PACKETBLOBS;i++)
    oggpack_reset(vbi->packetblob[i]);
  
  



  if((ret=_mapping_P[0]->forward(vb)))
    return(ret);

  if(op){
    if(vorbis_bitrate_managed(vb))
      

      return(OV_EINVAL);
    
    op->packet=oggpack_get_buffer(&vb->opb);
    op->bytes=oggpack_bytes(&vb->opb);
    op->b_o_s=0;
    op->e_o_s=vb->eofflag;
    op->granulepos=vb->granulepos;
    op->packetno=vb->sequence; 
  }
  return(0);
}


void _analysis_output_always(char *base,int i,float *v,int n,int bark,int dB,ogg_int64_t off){
#if 0
  int j;
  FILE *of;
  char buffer[80];

  
    sprintf(buffer,"%s_%d.m",base,i);
    of=fopen(buffer,"w");
    
    if(!of)perror("failed to open data dump file");
    
    for(j=0;j<n;j++){
      if(bark){
        float b=toBARK((4000.f*j/n)+.25);
        fprintf(of,"%f ",b);
      }else
        if(off!=0)
          fprintf(of,"%f ",(double)(j+off)/8000.);
        else
          fprintf(of,"%f ",(double)j);
      
      if(dB){
        float val;
        if(v[j]==0.)
          val=-140.;
        else
          val=todB(v+j);
        fprintf(of,"%f\n",val);
      }else{
        fprintf(of,"%f\n",v[j]);
      }
    }
    fclose(of);
    
#endif
}

void _analysis_output(char *base,int i,float *v,int n,int bark,int dB,
                      ogg_int64_t off){
  if(analysis_noisy)_analysis_output_always(base,i,v,n,bark,dB,off);
}













