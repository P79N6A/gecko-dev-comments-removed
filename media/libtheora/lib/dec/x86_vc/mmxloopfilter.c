


























#include "../../internal.h"
#include "x86int.h"
#include <mmintrin.h>

#if defined(USE_ASM)



static void loop_filter_v(unsigned char *_pix,int _ystride,
                          const ogg_int16_t *_ll){
  _asm {
    mov       eax,  [_pix]
    mov       edx,  [_ystride]
    mov       ecx,  [_ll]

    
    sub       eax,   edx
    
    pxor      mm0,   mm0
    
    sub       eax,   edx
    
    lea       esi, [edx + edx*2]

    
    movq      mm7, [eax]
    
    movq      mm4, [eax + esi]
    
    movq      mm6, mm7
    
    punpcklbw mm6, mm0
    movq      mm5, mm4
    
    punpckhbw mm7, mm0
    punpcklbw mm4, mm0
    
    punpckhbw mm5, mm0
    
    psubw     mm6, mm4
    psubw     mm7, mm5
    
    movq      mm4, [eax + edx]
    
    movq      mm2, [eax + edx*2]
    movq      mm5, mm4
    movq      mm3, mm2
    movq      mm1, mm2
    
    punpckhbw mm5, mm0
    punpcklbw mm4, mm0
    punpckhbw mm3, mm0
    punpcklbw mm2, mm0
    pcmpeqw   mm0, mm0
    

    psubw     mm3, mm5
    psrlw     mm0, 14
    psubw     mm2, mm4
    
    pmullw    mm3, mm0
    pmullw    mm2, mm0
    


    psrlw     mm0, 1
    paddw     mm3, mm7
    psllw     mm0, 2
    paddw     mm2, mm6
    
    paddw     mm3, mm0
    paddw     mm2, mm0
    
    psraw     mm3, 3
    psraw     mm2, 3
    
    
    packuswb  mm4, mm5
    
    movq      mm0, [ecx]
    
    movq      mm5, mm2
    pxor      mm6, mm6
    movq      mm7, mm0
    psubw     mm6, mm0
    psllw     mm7, 1
    psllw     mm6, 1
    
    
    
    
    pcmpgtw   mm7, mm2
    pcmpgtw   mm5, mm6
    pand      mm2, mm7
    movq      mm7, mm0
    pand      mm2, mm5
    psllw     mm7, 1
    movq      mm5, mm3
    
    
    
    
    pcmpgtw   mm7, mm3
    pcmpgtw   mm5, mm6
    pand      mm3, mm7
    movq      mm7, mm0
    pand      mm3, mm5
   


    psraw     mm6, 1
    movq      mm5, mm2
    psllw     mm7, 1
    
    
    
    
    
    pcmpgtw   mm5, mm0
    
    pcmpgtw   mm6, mm2
    
    pand      mm7, mm5
    
    psubw     mm2, mm7
    movq      mm7, mm0
    
    por       mm5, mm6
    psllw     mm7, 1
    
    pand      mm7, mm6
    pxor      mm6, mm6
    
    paddw     mm2, mm7
    psubw     mm6, mm0
    
    pand      mm5, mm2
    movq      mm7, mm0
    
    psubw     mm2, mm5
    psllw     mm7, 1
    
    psubw     mm2, mm5
    movq      mm5, mm3
    
    
    
    
    
    pcmpgtw   mm6, mm3
    
    pcmpgtw   mm5, mm0
    
    pand      mm7, mm5
    
    psubw     mm3, mm7
    psllw     mm0, 1
    
    por       mm5, mm6
    
    pand      mm0, mm6
    
    paddw     mm3, mm0
    
    pand      mm5, mm3
    
    psubw     mm3, mm5
    
    psubw     mm3, mm5
    

    pxor      mm0, mm0
    movq      mm5, mm4
    punpcklbw mm4, mm0
    punpckhbw mm5, mm0
    movq      mm6, mm1
    punpcklbw mm1, mm0
    punpckhbw mm6, mm0
    
    paddw     mm4, mm2
    paddw     mm5, mm3
    
    psubw     mm1, mm2
    psubw     mm6, mm3
    packuswb  mm4, mm5
    packuswb  mm1, mm6
    
    movq    [eax + edx], mm4
    movq    [eax + edx*2], mm1
  }
}





static void loop_filter_h4(unsigned char *_pix,long _ystride,
                           const ogg_int16_t *_ll){
  
  _asm {
    mov   ecx, [_pix]
    mov   edx, [_ystride]
    mov   eax, [_ll]
    
    lea     esi, [edx + edx*2]

    movd    mm0, dword ptr [ecx]
    movd    mm1, dword ptr [ecx + edx]
    movd    mm2, dword ptr [ecx + edx*2]
    movd    mm3, dword ptr [ecx + esi]
    punpcklbw mm0, mm1
    punpcklbw mm2, mm3
    movq    mm1, mm0
    punpckhwd mm0, mm2
    punpcklwd mm1, mm2
    pxor    mm7, mm7
    movq    mm5, mm1
    punpcklbw mm1, mm7
    punpckhbw mm5, mm7
    movq    mm3, mm0
    punpcklbw mm0, mm7
    punpckhbw mm3, mm7
    psubw   mm1, mm3
    movq    mm4, mm0
    pcmpeqw mm2, mm2
    psubw   mm0, mm5
    psrlw   mm2, 14
    pmullw  mm0, mm2
    psrlw   mm2, 1
    paddw   mm0, mm1
    psllw   mm2, 2
    paddw   mm0, mm2
    psraw   mm0, 3
    movq    mm6, qword ptr [eax]
    movq    mm1, mm0
    pxor    mm2, mm2
    movq    mm3, mm6
    psubw   mm2, mm6
    psllw   mm3, 1
    psllw   mm2, 1
    pcmpgtw mm3, mm0
    pcmpgtw mm1, mm2
    pand    mm0, mm3
    pand    mm0, mm1
    psraw   mm2, 1
    movq    mm1, mm0
    movq    mm3, mm6
    pcmpgtw mm2, mm0
    pcmpgtw mm1, mm6
    psllw   mm3, 1
    psllw   mm6, 1
    pand    mm3, mm1
    pand    mm6, mm2
    psubw   mm0, mm3
    por     mm1, mm2
    paddw   mm0, mm6
    pand    mm1, mm0
    psubw   mm0, mm1
    psubw   mm0, mm1
    paddw   mm5, mm0
    psubw   mm4, mm0
    packuswb mm5, mm7
    packuswb mm4, mm7
    punpcklbw mm5, mm4
    movd    edi, mm5
    mov     word ptr [ecx + 01H], di
    psrlq   mm5, 32
    shr     edi, 16
    mov     word ptr [ecx + edx + 01H], di
    movd    edi, mm5
    mov     word ptr [ecx + edx*2 + 01H], di
    shr     edi, 16
    mov     word ptr [ecx + esi + 01H], di
  }
}

static void loop_filter_h(unsigned char *_pix,int _ystride,
                          const ogg_int16_t *_ll){
  _pix-=2;
  loop_filter_h4(_pix,_ystride,_ll);
  loop_filter_h4(_pix+(_ystride<<2),_ystride,_ll);
}















void oc_state_loop_filter_frag_rows_mmx(oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end){
  ogg_int16_t __declspec(align(8))        ll[4];
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

  
  _mm_empty();
}

#endif
