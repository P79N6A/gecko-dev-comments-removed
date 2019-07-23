


















#include "x86int.h"
#include "../../internal.h"
#include <stddef.h>

#if defined(USE_ASM)

static const __attribute__((aligned(8),used)) int OC_FZIG_ZAGMMX[64]={
   0, 8, 1, 2, 9,16,24,17,
  10, 3,32,11,18,25, 4,12,
   5,26,19,40,33,34,41,48,
  27, 6,13,20,28,21,14, 7,
  56,49,42,35,43,50,57,36,
  15,22,29,30,23,44,37,58,
  51,59,38,45,52,31,60,53,
  46,39,47,54,61,62,55,63
};



void oc_state_frag_recon_mmx(oc_theora_state *_state,oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,int _ncoefs,
 ogg_uint16_t _dc_iquant,const ogg_uint16_t _ac_iquant[64]){
  ogg_int16_t  __attribute__((aligned(8))) res_buf[64];
  int dst_framei;
  int dst_ystride;
  int zzi;
  























  
  if(_last_zzi<2){
    ogg_uint16_t p;
    

    p=(ogg_int16_t)((ogg_int32_t)_frag->dc*_dc_iquant+15>>5);
    
    __asm__ __volatile__(
      
      "movd %[p],%%mm0\n\t"
      
      "movd %[p],%%mm1\n\t"
      
      "pslld $16,%%mm0\n\t"
      
      "por %%mm1,%%mm0\n\t"
      
      "punpcklwd %%mm0,%%mm0\n\t"
      "movq %%mm0,(%[res_buf])\n\t"
      "movq %%mm0,8(%[res_buf])\n\t"
      "movq %%mm0,16(%[res_buf])\n\t"
      "movq %%mm0,24(%[res_buf])\n\t"
      "movq %%mm0,32(%[res_buf])\n\t"
      "movq %%mm0,40(%[res_buf])\n\t"
      "movq %%mm0,48(%[res_buf])\n\t"
      "movq %%mm0,56(%[res_buf])\n\t"
      "movq %%mm0,64(%[res_buf])\n\t"
      "movq %%mm0,72(%[res_buf])\n\t"
      "movq %%mm0,80(%[res_buf])\n\t"
      "movq %%mm0,88(%[res_buf])\n\t"
      "movq %%mm0,96(%[res_buf])\n\t"
      "movq %%mm0,104(%[res_buf])\n\t"
      "movq %%mm0,112(%[res_buf])\n\t"
      "movq %%mm0,120(%[res_buf])\n\t"
      :
      :[res_buf]"r"(res_buf),[p]"r"((unsigned)p)
      :"memory"
    );
  }
  else{
    

    
    
    __asm__ __volatile__(
      "pxor %%mm0,%%mm0\n\t"
      "movq %%mm0,(%[res_buf])\n\t"
      "movq %%mm0,8(%[res_buf])\n\t"
      "movq %%mm0,16(%[res_buf])\n\t"
      "movq %%mm0,24(%[res_buf])\n\t"
      "movq %%mm0,32(%[res_buf])\n\t"
      "movq %%mm0,40(%[res_buf])\n\t"
      "movq %%mm0,48(%[res_buf])\n\t"
      "movq %%mm0,56(%[res_buf])\n\t"
      "movq %%mm0,64(%[res_buf])\n\t"
      "movq %%mm0,72(%[res_buf])\n\t"
      "movq %%mm0,80(%[res_buf])\n\t"
      "movq %%mm0,88(%[res_buf])\n\t"
      "movq %%mm0,96(%[res_buf])\n\t"
      "movq %%mm0,104(%[res_buf])\n\t"
      "movq %%mm0,112(%[res_buf])\n\t"
      "movq %%mm0,120(%[res_buf])\n\t"
      :
      :[res_buf]"r"(res_buf)
      :"memory"
    );
    res_buf[0]=(ogg_int16_t)((ogg_int32_t)_frag->dc*_dc_iquant);
    
    for(zzi=1;zzi<_ncoefs;zzi++){
      int ci;
      ci=OC_FZIG_ZAG[zzi];
      res_buf[OC_FZIG_ZAGMMX[zzi]]=(ogg_int16_t)((ogg_int32_t)_dct_coeffs[zzi]*
       _ac_iquant[ci]);
    }
    if(_last_zzi<10)oc_idct8x8_10_mmx(res_buf);
    else oc_idct8x8_mmx(res_buf);
  }
  
  dst_framei=_state->ref_frame_idx[OC_FRAME_SELF];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].stride;
  
  if(_frag->mbmode==OC_MODE_INTRA){
    oc_frag_recon_intra_mmx(_frag->buffer[dst_framei],dst_ystride,res_buf);
  }
  else{
    int ref_framei;
    int ref_ystride;
    int mvoffsets[2];
    ref_framei=_state->ref_frame_idx[OC_FRAME_FOR_MODE[_frag->mbmode]];
    ref_ystride=_state->ref_frame_bufs[ref_framei][_pli].stride;
    if(oc_state_get_mv_offsets(_state,mvoffsets,_frag->mv[0],_frag->mv[1],
     ref_ystride,_pli)>1){
      oc_frag_recon_inter2_mmx(_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[ref_framei]+mvoffsets[0],ref_ystride,
       _frag->buffer[ref_framei]+mvoffsets[1],ref_ystride,res_buf);
    }
    else{
      oc_frag_recon_inter_mmx(_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[ref_framei]+mvoffsets[0],ref_ystride,res_buf);
    }
  }
  oc_restore_fpu(_state);
}








void oc_state_frag_copy_mmx(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli){
  const int *fragi;
  const int *fragi_end;
  int        dst_framei;
  ptrdiff_t  dst_ystride;
  int        src_framei;
  ptrdiff_t  src_ystride;
  dst_framei=_state->ref_frame_idx[_dst_frame];
  src_framei=_state->ref_frame_idx[_src_frame];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].stride;
  src_ystride=_state->ref_frame_bufs[src_framei][_pli].stride;
  fragi_end=_fragis+_nfragis;
  for(fragi=_fragis;fragi<fragi_end;fragi++){
    oc_fragment   *frag;
    unsigned char *dst;
    unsigned char *src;
    ptrdiff_t      s;
    frag=_state->frags+*fragi;
    dst=frag->buffer[dst_framei];
    src=frag->buffer[src_framei];
    __asm__ __volatile__(
      
      "movq (%[src]),%%mm0\n\t"
      
      "lea (%[src_ystride],%[src_ystride],2),%[s]\n\t"
      
      "movq (%[src],%[src_ystride]),%%mm1\n\t"
      
      "movq (%[src],%[src_ystride],2),%%mm2\n\t"
      
      "movq (%[src],%[s]),%%mm3\n\t"
      
      "movq %%mm0,(%[dst])\n\t"
      
      "lea (%[dst_ystride],%[dst_ystride],2),%[s]\n\t"
      
      "movq %%mm1,(%[dst],%[dst_ystride])\n\t"
      
      "lea (%[src],%[src_ystride],4),%[src]\n\t"
      
      "movq %%mm2,(%[dst],%[dst_ystride],2)\n\t"
      
      "movq %%mm3,(%[dst],%[s])\n\t"
      
      "lea (%[dst],%[dst_ystride],4),%[dst]\n\t"
      
      "movq (%[src]),%%mm0\n\t"
      
      "lea (%[src_ystride],%[src_ystride],2),%[s]\n\t"
      
      "movq (%[src],%[src_ystride]),%%mm1\n\t"
      
      "movq (%[src],%[src_ystride],2),%%mm2\n\t"
      
      "movq (%[src],%[s]),%%mm3\n\t"
      
      "movq %%mm0,(%[dst])\n\t"
      
      "lea (%[dst_ystride],%[dst_ystride],2),%[s]\n\t"
      
      "movq %%mm1,(%[dst],%[dst_ystride])\n\t"
      
      "movq %%mm2,(%[dst],%[dst_ystride],2)\n\t"
      
      "movq %%mm3,(%[dst],%[s])\n\t"
      :[s]"=&r"(s)
      :[dst]"r"(dst),[src]"r"(src),[dst_ystride]"r"(dst_ystride),
       [src_ystride]"r"(src_ystride)
      :"memory"
    );
  }
  
  __asm__ __volatile__("emms\n\t");
}

static void loop_filter_v(unsigned char *_pix,int _ystride,
 const ogg_int16_t *_ll){
  ptrdiff_t s;
  _pix-=_ystride*2;
  __asm__ __volatile__(
    
    "pxor %%mm0,%%mm0\n\t"
    
    "lea (%[ystride],%[ystride],2),%[s]\n\t"
    
    "movq (%[pix]),%%mm7\n\t"
    
    "movq (%[pix],%[s]),%%mm4\n\t"
    
    "movq %%mm7,%%mm6\n\t"
    
    "punpcklbw %%mm0,%%mm6\n\t"
    "movq %%mm4,%%mm5\n\t"
    
    "punpckhbw %%mm0,%%mm7\n\t"
    
    "punpcklbw %%mm0,%%mm4\n\t"
    "punpckhbw %%mm0,%%mm5\n\t"
    
    "psubw %%mm4,%%mm6\n\t"
    "psubw %%mm5,%%mm7\n\t"
    
    "movq (%[pix],%[ystride]),%%mm4\n\t"
    
    "movq (%[pix],%[ystride],2),%%mm2\n\t"
    "movq %%mm4,%%mm5\n\t"
    "movq %%mm2,%%mm3\n\t"
    "movq %%mm2,%%mm1\n\t"
    
    "punpckhbw %%mm0,%%mm5\n\t"
    "punpcklbw %%mm0,%%mm4\n\t"
    "punpckhbw %%mm0,%%mm3\n\t"
    "punpcklbw %%mm0,%%mm2\n\t"
    

    "pcmpeqw %%mm0,%%mm0\n\t"
    "psubw %%mm5,%%mm3\n\t"
    "psrlw $14,%%mm0\n\t"
    "psubw %%mm4,%%mm2\n\t"
    
    "pmullw %%mm0,%%mm3\n\t"
    "pmullw %%mm0,%%mm2\n\t"
    


    "psrlw $1,%%mm0\n\t"
    "paddw %%mm7,%%mm3\n\t"
    "psllw $2,%%mm0\n\t"
    "paddw %%mm6,%%mm2\n\t"
    
    "paddw %%mm0,%%mm3\n\t"
    "paddw %%mm0,%%mm2\n\t"
    
    "psraw $3,%%mm3\n\t"
    "psraw $3,%%mm2\n\t"
    
    
    "packuswb %%mm5,%%mm4\n\t"
    
    "movq (%[ll]),%%mm0\n\t"
    
    "movq %%mm2,%%mm5\n\t"
    "pxor %%mm6,%%mm6\n\t"
    "movq %%mm0,%%mm7\n\t"
    "psubw %%mm0,%%mm6\n\t"
    "psllw $1,%%mm7\n\t"
    "psllw $1,%%mm6\n\t"
    
    
    
    
    "pcmpgtw %%mm2,%%mm7\n\t"
    "pcmpgtw %%mm6,%%mm5\n\t"
    "pand %%mm7,%%mm2\n\t"
    "movq %%mm0,%%mm7\n\t"
    "pand %%mm5,%%mm2\n\t"
    "psllw $1,%%mm7\n\t"
    "movq %%mm3,%%mm5\n\t"
    
    
    
    
    "pcmpgtw %%mm3,%%mm7\n\t"
    "pcmpgtw %%mm6,%%mm5\n\t"
    "pand %%mm7,%%mm3\n\t"
    "movq %%mm0,%%mm7\n\t"
    "pand %%mm5,%%mm3\n\t"
    


    "psraw $1,%%mm6\n\t"
    "movq %%mm2,%%mm5\n\t"
    "psllw $1,%%mm7\n\t"
    
    
    
    
    
    "pcmpgtw %%mm0,%%mm5\n\t"
    
    "pcmpgtw %%mm2,%%mm6\n\t"
    
    "pand %%mm5,%%mm7\n\t"
    
    "psubw %%mm7,%%mm2\n\t"
    "movq %%mm0,%%mm7\n\t"
    
    "por %%mm6,%%mm5\n\t"
    "psllw $1,%%mm7\n\t"
    
    "pand %%mm6,%%mm7\n\t"
    "pxor %%mm6,%%mm6\n\t"
    
    "paddw %%mm7,%%mm2\n\t"
    "psubw %%mm0,%%mm6\n\t"
    
    "pand %%mm2,%%mm5\n\t"
    "movq %%mm0,%%mm7\n\t"
    
    "psubw %%mm5,%%mm2\n\t"
    "psllw $1,%%mm7\n\t"
    
    "psubw %%mm5,%%mm2\n\t"
    "movq %%mm3,%%mm5\n\t"
    
    
    
    
    
    "pcmpgtw %%mm3,%%mm6\n\t"
    
    "pcmpgtw %%mm0,%%mm5\n\t"
    
    "pand %%mm5,%%mm7\n\t"
    
    "psubw %%mm7,%%mm3\n\t"
    "psllw $1,%%mm0\n\t"
    
    "por %%mm6,%%mm5\n\t"
    
    "pand %%mm6,%%mm0\n\t"
    
    "paddw %%mm0,%%mm3\n\t"
    
    "pand %%mm3,%%mm5\n\t"
    
    "psubw %%mm5,%%mm3\n\t"
    
    "psubw %%mm5,%%mm3\n\t"
    

    "pxor %%mm0,%%mm0\n\t"
    "movq %%mm4,%%mm5\n\t"
    "punpcklbw %%mm0,%%mm4\n\t"
    "punpckhbw %%mm0,%%mm5\n\t"
    "movq %%mm1,%%mm6\n\t"
    "punpcklbw %%mm0,%%mm1\n\t"
    "punpckhbw %%mm0,%%mm6\n\t"
    
    "paddw %%mm2,%%mm4\n\t"
    "paddw %%mm3,%%mm5\n\t"
    
    "psubw %%mm2,%%mm1\n\t"
    "psubw %%mm3,%%mm6\n\t"
    "packuswb %%mm5,%%mm4\n\t"
    "packuswb %%mm6,%%mm1\n\t"
    
    "movq %%mm4,(%[pix],%[ystride])\n\t"
    "movq %%mm1,(%[pix],%[ystride],2)\n\t"
    :[s]"=&r"(s)
    :[pix]"r"(_pix),[ystride]"r"((ptrdiff_t)_ystride),[ll]"r"(_ll)
    :"memory"
  );
}





static void loop_filter_h4(unsigned char *_pix,ptrdiff_t _ystride,
 const ogg_int16_t *_ll){
  ptrdiff_t s;
  

  ptrdiff_t d;
  __asm__ __volatile__(
    
    "movd (%[pix]),%%mm0\n\t"
    
    "lea (%[ystride],%[ystride],2),%[s]\n\t"
    
    "movd (%[pix],%[ystride]),%%mm1\n\t"
    
    "movd (%[pix],%[ystride],2),%%mm2\n\t"
    
    "movd (%[pix],%[s]),%%mm3\n\t"
    
    "punpcklbw %%mm1,%%mm0\n\t"
    
    "punpcklbw %%mm3,%%mm2\n\t"
    
    "movq %%mm0,%%mm1\n\t"
    
    "punpckhwd %%mm2,%%mm0\n\t"
    
    "punpcklwd %%mm2,%%mm1\n\t"
    "pxor %%mm7,%%mm7\n\t"
    
    "movq %%mm1,%%mm5\n\t"
    
    "punpcklbw %%mm7,%%mm1\n\t"
    
    "punpckhbw %%mm7,%%mm5\n\t"
    
    "movq %%mm0,%%mm3\n\t"
    
    "punpcklbw %%mm7,%%mm0\n\t"
    
    "punpckhbw %%mm7,%%mm3\n\t"
    
    "psubw %%mm3,%%mm1\n\t"
    
    "movq %%mm0,%%mm4\n\t"
    

    "pcmpeqw %%mm2,%%mm2\n\t"
    "psubw %%mm5,%%mm0\n\t"
    "psrlw $14,%%mm2\n\t"
    
    "pmullw %%mm2,%%mm0\n\t"
    

    "psrlw $1,%%mm2\n\t"
    "paddw %%mm1,%%mm0\n\t"
    "psllw $2,%%mm2\n\t"
    
    "paddw %%mm2,%%mm0\n\t"
    
    "psraw $3,%%mm0\n\t"
    
    
    "movq (%[ll]),%%mm6\n\t"
    
    "movq %%mm0,%%mm1\n\t"
    "pxor %%mm2,%%mm2\n\t"
    "movq %%mm6,%%mm3\n\t"
    "psubw %%mm6,%%mm2\n\t"
    "psllw $1,%%mm3\n\t"
    "psllw $1,%%mm2\n\t"
    
    
    
    
    "pcmpgtw %%mm0,%%mm3\n\t"
    "pcmpgtw %%mm2,%%mm1\n\t"
    "pand %%mm3,%%mm0\n\t"
    "pand %%mm1,%%mm0\n\t"
    


    "psraw $1,%%mm2\n\t"
    "movq %%mm0,%%mm1\n\t"
    "movq %%mm6,%%mm3\n\t"
    
    
    
    
    
    "pcmpgtw %%mm0,%%mm2\n\t"
    
    "pcmpgtw %%mm6,%%mm1\n\t"
    
    "psllw $1,%%mm3\n\t"
    
    "psllw $1,%%mm6\n\t"
    
    "pand %%mm1,%%mm3\n\t"
    
    "pand %%mm2,%%mm6\n\t"
    
    "psubw %%mm3,%%mm0\n\t"
    
    "por %%mm2,%%mm1\n\t"
    
    "paddw %%mm6,%%mm0\n\t"
    
    "pand %%mm0,%%mm1\n\t"
    
    "psubw %%mm1,%%mm0\n\t"
    
    "psubw %%mm1,%%mm0\n\t"
    
    "paddw %%mm0,%%mm5\n\t"
    
    "psubw %%mm0,%%mm4\n\t"
    
    "packuswb %%mm7,%%mm5\n\t"
    
    "packuswb %%mm7,%%mm4\n\t"
    
    "punpcklbw %%mm4,%%mm5\n\t"
    
    "movd %%mm5,%[d]\n\t"
    "movw %w[d],1(%[pix])\n\t"
    
    "psrlq $32,%%mm5\n\t"
    "shr $16,%[d]\n\t"
    "movw %w[d],1(%[pix],%[ystride])\n\t"
    
    "movd %%mm5,%[d]\n\t"
    "movw %w[d],1(%[pix],%[ystride],2)\n\t"
    "shr $16,%[d]\n\t"
    "movw %w[d],1(%[pix],%[s])\n\t"
    :[s]"=&r"(s),[d]"=&r"(d),
     [pix]"+r"(_pix),[ystride]"+r"(_ystride),[ll]"+r"(_ll)
    :
    :"memory"
  );
}

static void loop_filter_h(unsigned char *_pix,int _ystride,
 const ogg_int16_t *_ll){
  _pix-=2;
  loop_filter_h4(_pix,_ystride,_ll);
  loop_filter_h4(_pix+(_ystride<<2),_ystride,_ll);
}














void oc_state_loop_filter_frag_rows_mmx(oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end){
  ogg_int16_t __attribute__((aligned(8)))  ll[4];
  th_img_plane                            *iplane;
  oc_fragment_plane                       *fplane;
  oc_fragment                             *frag_top;
  oc_fragment                             *frag0;
  oc_fragment                             *frag;
  oc_fragment                             *frag_end;
  oc_fragment                             *frag0_end;
  oc_fragment                             *frag_bot;
  ll[0]=ll[1]=ll[2]=ll[3]=
   (ogg_int16_t)_state->loop_filter_limits[_state->qis[0]];
  iplane=_state->ref_frame_bufs[_refi]+_pli;
  fplane=_state->fplanes+_pli;
  




  frag_top=_state->frags+fplane->froffset;
  frag0=frag_top+_fragy0*fplane->nhfrags;
  frag0_end=frag0+(_fragy_end-_fragy0)*fplane->nhfrags;
  frag_bot=_state->frags+fplane->froffset+fplane->nfrags;
  while(frag0<frag0_end){
    frag=frag0;
    frag_end=frag+fplane->nhfrags;
    while(frag<frag_end){
      if(frag->coded){
        if(frag>frag0){
          loop_filter_h(frag->buffer[_refi],iplane->stride,ll);
        }
        if(frag0>frag_top){
          loop_filter_v(frag->buffer[_refi],iplane->stride,ll);
        }
        if(frag+1<frag_end&&!(frag+1)->coded){
          loop_filter_h(frag->buffer[_refi]+8,iplane->stride,ll);
        }
        if(frag+fplane->nhfrags<frag_bot&&!(frag+fplane->nhfrags)->coded){
          loop_filter_v((frag+fplane->nhfrags)->buffer[_refi],
           iplane->stride,ll);
        }
      }
      frag++;
    }
    frag0+=fplane->nhfrags;
  }
  
  __asm__ __volatile__("emms\n\t");
}

#endif
