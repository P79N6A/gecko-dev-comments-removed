


















#include <string.h>
#include "x86int.h"
#include "mmxloop.h"

#if defined(OC_X86_ASM)

void oc_state_frag_recon_mmx(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,ogg_uint16_t _dc_quant){
  unsigned char *dst;
  ptrdiff_t      frag_buf_off;
  int            ystride;
  int            refi;
  
  
  if(_last_zzi<2){
    

    ogg_uint16_t p;
    

    p=(ogg_int16_t)(_dct_coeffs[0]*(ogg_int32_t)_dc_quant+15>>5);
    
    __asm{
#define Y eax
#define P ecx
      mov Y,_dct_coeffs
      movzx P,p
      lea Y,[Y+128]
      
      movd mm0,P
      
      punpcklwd mm0,mm0
      
      punpckldq mm0,mm0
      movq [Y],mm0
      movq [8+Y],mm0
      movq [16+Y],mm0
      movq [24+Y],mm0
      movq [32+Y],mm0
      movq [40+Y],mm0
      movq [48+Y],mm0
      movq [56+Y],mm0
      movq [64+Y],mm0
      movq [72+Y],mm0
      movq [80+Y],mm0
      movq [88+Y],mm0
      movq [96+Y],mm0
      movq [104+Y],mm0
      movq [112+Y],mm0
      movq [120+Y],mm0
#undef Y
#undef P
    }
  }
  else{
    
    _dct_coeffs[0]=(ogg_int16_t)(_dct_coeffs[0]*(int)_dc_quant);
    oc_idct8x8_mmx(_dct_coeffs+64,_dct_coeffs,_last_zzi);
  }
  
  frag_buf_off=_state->frag_buf_offs[_fragi];
  refi=_state->frags[_fragi].refi;
  ystride=_state->ref_ystride[_pli];
  dst=_state->ref_frame_data[OC_FRAME_SELF]+frag_buf_off;
  if(refi==OC_FRAME_SELF)oc_frag_recon_intra_mmx(dst,ystride,_dct_coeffs+64);
  else{
    const unsigned char *ref;
    int                  mvoffsets[2];
    ref=_state->ref_frame_data[refi]+frag_buf_off;
    if(oc_state_get_mv_offsets(_state,mvoffsets,_pli,
     _state->frag_mvs[_fragi])>1){
      oc_frag_recon_inter2_mmx(dst,ref+mvoffsets[0],ref+mvoffsets[1],ystride,
       _dct_coeffs+64);
    }
    else oc_frag_recon_inter_mmx(dst,ref+mvoffsets[0],ystride,_dct_coeffs+64);
  }
}




void oc_loop_filter_init_mmx(signed char _bv[256],int _flimit){
  memset(_bv,~(_flimit<<1),8);
}









void oc_state_loop_filter_frag_rows_mmx(const oc_theora_state *_state,
 signed char _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end){
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
  fplane=_state->fplanes+_pli;
  nhfrags=fplane->nhfrags;
  fragi_top=fplane->froffset;
  fragi_bot=fragi_top+fplane->nfrags;
  fragi0=fragi_top+_fragy0*(ptrdiff_t)nhfrags;
  fragi0_end=fragi_top+_fragy_end*(ptrdiff_t)nhfrags;
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
#define PIX eax
#define YSTRIDE3 edi
#define YSTRIDE ecx
#define LL edx
#define D esi
#define D_WORD si
        if(fragi>fragi0)OC_LOOP_FILTER_H_MMX(ref,ystride,_bv);
        if(fragi0>fragi_top)OC_LOOP_FILTER_V_MMX(ref,ystride,_bv);
        if(fragi+1<fragi_end&&!frags[fragi+1].coded){
          OC_LOOP_FILTER_H_MMX(ref+8,ystride,_bv);
        }
        if(fragi+nhfrags<fragi_bot&&!frags[fragi+nhfrags].coded){
          OC_LOOP_FILTER_V_MMX(ref+(ystride<<3),ystride,_bv);
        }
#undef PIX
#undef YSTRIDE3
#undef YSTRIDE
#undef LL
#undef D
#undef D_WORD
      }
      fragi++;
    }
    fragi0+=nhfrags;
  }
}

#endif
