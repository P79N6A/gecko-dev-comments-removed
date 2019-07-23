
















#include "../internal.h"

void oc_frag_recon_intra(const oc_theora_state *_state,unsigned char *_dst,
 int _dst_ystride,const ogg_int16_t *_residue){
  _state->opt_vtable.frag_recon_intra(_dst,_dst_ystride,_residue);
}

void oc_frag_recon_intra_c(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t *_residue){
  int i;
  for(i=0;i<8;i++){
    int j;
    for(j=0;j<8;j++){
      int res;
      res=*_residue++;
      _dst[j]=OC_CLAMP255(res+128);
    }
    _dst+=_dst_ystride;
  }
}

void oc_frag_recon_inter(const oc_theora_state *_state,unsigned char *_dst,
 int _dst_ystride,const unsigned char *_src,int _src_ystride,
 const ogg_int16_t *_residue){
  _state->opt_vtable.frag_recon_inter(_dst,_dst_ystride,_src,_src_ystride,
   _residue);
}

void oc_frag_recon_inter_c(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue){
  int i;
  for(i=0;i<8;i++){
    int j;
    for(j=0;j<8;j++){
      int res;
      res=*_residue++;
      _dst[j]=OC_CLAMP255(res+_src[j]);
    }
    _dst+=_dst_ystride;
    _src+=_src_ystride;
  }
}

void oc_frag_recon_inter2(const oc_theora_state *_state,unsigned char *_dst,
 int _dst_ystride,const unsigned char *_src1,int _src1_ystride,
 const unsigned char *_src2,int _src2_ystride,const ogg_int16_t *_residue){
  _state->opt_vtable.frag_recon_inter2(_dst,_dst_ystride,_src1,_src1_ystride,
   _src2,_src2_ystride,_residue);
}

void oc_frag_recon_inter2_c(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue){
  int i;
  for(i=0;i<8;i++){
    int j;
    for(j=0;j<8;j++){
      int res;
      res=*_residue++;
      _dst[j]=OC_CLAMP255(res+((int)_src1[j]+_src2[j]>>1));
    }
    _dst+=_dst_ystride;
    _src1+=_src1_ystride;
    _src2+=_src2_ystride;
  }
}













int oc_frag_pred_dc(const oc_fragment *_frag,
 const oc_fragment_plane *_fplane,int _x,int _y,int _pred_last[3]){
  static const int PRED_SCALE[16][4]={
    
    {0,0,0,0},
    
    {1,0,0,0},
    
    {1,0,0,0},
    
    {1,0,0,0},
    
    {1,0,0,0},
    
    {1,1,0,0},
    
    {0,1,0,0},
    
    {29,-26,29,0},
    
    {1,0,0,0},
    
    {75,53,0,0},
    
    {1,1,0,0},
    
    {75,0,53,0},
    
    {1,0,0,0},
    
    {75,0,53,0},
    
    {3,10,3,0},
    
    {29,-26,29,0}
  };
  static const int PRED_SHIFT[16]={0,0,0,0,0,1,0,5,0,7,1,7,0,7,4,5};
  static const int PRED_RMASK[16]={0,0,0,0,0,1,0,31,0,127,1,127,0,127,15,31};
  static const int BC_MASK[8]={
    
    OC_PL|OC_PUL|OC_PU|OC_PUR,
    
    OC_PU|OC_PUR,
    
    OC_PL,
    
    0,
    
    OC_PL|OC_PUL|OC_PU,
    
    OC_PU,
    
    OC_PL,
    
    0
  };
  
  const oc_fragment *predfr[4];
  
  int                pred_frame;
  
  int                bc;
  

  int                p[4];
  
  int                np;
  
  int                pflags;
  
  int                ret;
  int                i;
  pred_frame=OC_FRAME_FOR_MODE[_frag->mbmode];
  bc=(_x==0)+((_y==0)<<1)+((_x+1==_fplane->nhfrags)<<2);
  predfr[0]=_frag-1;
  predfr[1]=_frag-_fplane->nhfrags-1;
  predfr[2]=predfr[1]+1;
  predfr[3]=predfr[2]+1;
  np=0;
  pflags=0;
  for(i=0;i<4;i++){
    int pflag;
    pflag=1<<i;
    if((BC_MASK[bc]&pflag)&&predfr[i]->coded&&
     OC_FRAME_FOR_MODE[predfr[i]->mbmode]==pred_frame){
      p[np++]=predfr[i]->dc;
      pflags|=pflag;
    }
  }
  if(pflags==0)return _pred_last[pred_frame];
  else{
    ret=PRED_SCALE[pflags][0]*p[0];
    
    for(i=1;i<np;i++)ret+=PRED_SCALE[pflags][i]*p[i];
    ret=OC_DIV_POW2(ret,PRED_SHIFT[pflags],PRED_RMASK[pflags]);
  }
  if((pflags&(OC_PL|OC_PUL|OC_PU))==(OC_PL|OC_PUL|OC_PU)){
    if(abs(ret-p[2])>128)ret=p[2];
    else if(abs(ret-p[0])>128)ret=p[0];
    else if(abs(ret-p[1])>128)ret=p[1];
  }
  return ret;
}
