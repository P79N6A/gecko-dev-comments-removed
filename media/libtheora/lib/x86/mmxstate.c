


















#include <string.h>
#include "x86int.h"
#include "mmxfrag.h"
#include "mmxloop.h"

#if defined(OC_X86_ASM)

void oc_state_frag_recon_mmx(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[64],int _last_zzi,ogg_uint16_t _dc_quant){
  unsigned char *dst;
  ptrdiff_t      frag_buf_off;
  int            ystride;
  int            mb_mode;
  
  
  if(_last_zzi<2){
    

    ogg_uint16_t p;
    

    p=(ogg_int16_t)(_dct_coeffs[0]*(ogg_int32_t)_dc_quant+15>>5);
    
    __asm__ __volatile__(
      
      "movd %[p],%%mm0\n\t"
      
      "punpcklwd %%mm0,%%mm0\n\t"
      
      "punpckldq %%mm0,%%mm0\n\t"
      "movq %%mm0,(%[y])\n\t"
      "movq %%mm0,8(%[y])\n\t"
      "movq %%mm0,16(%[y])\n\t"
      "movq %%mm0,24(%[y])\n\t"
      "movq %%mm0,32(%[y])\n\t"
      "movq %%mm0,40(%[y])\n\t"
      "movq %%mm0,48(%[y])\n\t"
      "movq %%mm0,56(%[y])\n\t"
      "movq %%mm0,64(%[y])\n\t"
      "movq %%mm0,72(%[y])\n\t"
      "movq %%mm0,80(%[y])\n\t"
      "movq %%mm0,88(%[y])\n\t"
      "movq %%mm0,96(%[y])\n\t"
      "movq %%mm0,104(%[y])\n\t"
      "movq %%mm0,112(%[y])\n\t"
      "movq %%mm0,120(%[y])\n\t"
      :
      :[y]"r"(_dct_coeffs),[p]"r"((unsigned)p)
      :"memory"
    );
  }
  else{
    
    _dct_coeffs[0]=(ogg_int16_t)(_dct_coeffs[0]*(int)_dc_quant);
    oc_idct8x8_mmx(_dct_coeffs,_last_zzi);
  }
  
  frag_buf_off=_state->frag_buf_offs[_fragi];
  mb_mode=_state->frags[_fragi].mb_mode;
  ystride=_state->ref_ystride[_pli];
  dst=_state->ref_frame_data[_state->ref_frame_idx[OC_FRAME_SELF]]+frag_buf_off;
  if(mb_mode==OC_MODE_INTRA)oc_frag_recon_intra_mmx(dst,ystride,_dct_coeffs);
  else{
    const unsigned char *ref;
    int                  mvoffsets[2];
    ref=
     _state->ref_frame_data[_state->ref_frame_idx[OC_FRAME_FOR_MODE(mb_mode)]]
     +frag_buf_off;
    if(oc_state_get_mv_offsets(_state,mvoffsets,_pli,
     _state->frag_mvs[_fragi][0],_state->frag_mvs[_fragi][1])>1){
      oc_frag_recon_inter2_mmx(dst,ref+mvoffsets[0],ref+mvoffsets[1],ystride,
       _dct_coeffs);
    }
    else oc_frag_recon_inter_mmx(dst,ref+mvoffsets[0],ystride,_dct_coeffs);
  }
}











void oc_state_frag_copy_list_mmx(const oc_theora_state *_state,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,
 int _dst_frame,int _src_frame,int _pli){
  const ptrdiff_t     *frag_buf_offs;
  const unsigned char *src_frame_data;
  unsigned char       *dst_frame_data;
  ptrdiff_t            fragii;
  int                  ystride;
  dst_frame_data=_state->ref_frame_data[_state->ref_frame_idx[_dst_frame]];
  src_frame_data=_state->ref_frame_data[_state->ref_frame_idx[_src_frame]];
  ystride=_state->ref_ystride[_pli];
  frag_buf_offs=_state->frag_buf_offs;
  for(fragii=0;fragii<_nfragis;fragii++){
    ptrdiff_t frag_buf_off;
    frag_buf_off=frag_buf_offs[_fragis[fragii]];
    OC_FRAG_COPY_MMX(dst_frame_data+frag_buf_off,
     src_frame_data+frag_buf_off,ystride);
  }
}









void oc_state_loop_filter_frag_rows_mmx(const oc_theora_state *_state,
 int _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end){
  OC_ALIGN8(unsigned char   ll[8]);
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  const ptrdiff_t         *frag_buf_offs;
  unsigned char           *ref_frame_data;
  ptrdiff_t                fragi_top;
  ptrdiff_t                fragi_bot;
  ptrdiff_t                fragi0;
  ptrdiff_t                fragi0_end;
  int                      ystride;
  int                      nhfrags;
  memset(ll,_state->loop_filter_limits[_state->qis[0]],sizeof(ll));
  fplane=_state->fplanes+_pli;
  nhfrags=fplane->nhfrags;
  fragi_top=fplane->froffset;
  fragi_bot=fragi_top+fplane->nfrags;
  fragi0=fragi_top+_fragy0*(ptrdiff_t)nhfrags;
  fragi0_end=fragi0+(_fragy_end-_fragy0)*(ptrdiff_t)nhfrags;
  ystride=_state->ref_ystride[_pli];
  frags=_state->frags;
  frag_buf_offs=_state->frag_buf_offs;
  ref_frame_data=_state->ref_frame_data[_refi];
  




  while(fragi0<fragi0_end){
    ptrdiff_t fragi;
    ptrdiff_t fragi_end;
    fragi=fragi0;
    fragi_end=fragi+nhfrags;
    while(fragi<fragi_end){
      if(frags[fragi].coded){
        unsigned char *ref;
        ref=ref_frame_data+frag_buf_offs[fragi];
        if(fragi>fragi0)OC_LOOP_FILTER_H_MMX(ref,ystride,ll);
        if(fragi0>fragi_top)OC_LOOP_FILTER_V_MMX(ref,ystride,ll);
        if(fragi+1<fragi_end&&!frags[fragi+1].coded){
          OC_LOOP_FILTER_H_MMX(ref+8,ystride,ll);
        }
        if(fragi+nhfrags<fragi_bot&&!frags[fragi+nhfrags].coded){
          OC_LOOP_FILTER_V_MMX(ref+(ystride<<3),ystride,ll);
        }
      }
      fragi++;
    }
    fragi0+=nhfrags;
  }
}

#endif
