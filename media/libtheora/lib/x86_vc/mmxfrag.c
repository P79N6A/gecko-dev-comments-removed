





















#include <stddef.h>
#include "x86int.h"
#include "mmxfrag.h"

#if defined(OC_X86_ASM)



void oc_frag_copy_mmx(unsigned char *_dst,
 const unsigned char *_src,int _ystride){
#define SRC edx
#define DST eax
#define YSTRIDE ecx
#define YSTRIDE3 esi
  OC_FRAG_COPY_MMX(_dst,_src,_ystride);
#undef SRC
#undef DST
#undef YSTRIDE
#undef YSTRIDE3
}

void oc_frag_recon_intra_mmx(unsigned char *_dst,int _ystride,
 const ogg_int16_t *_residue){
  __asm{
#define DST edx
#define DST4 esi
#define YSTRIDE eax
#define YSTRIDE3 edi
#define RESIDUE ecx
    mov DST,_dst
    mov YSTRIDE,_ystride
    mov RESIDUE,_residue
    lea DST4,[DST+YSTRIDE*4]
    lea YSTRIDE3,[YSTRIDE+YSTRIDE*2]
    
    pcmpeqw mm0,mm0
    
    movq mm1,[0*8+RESIDUE]
    
    movq mm2,[1*8+RESIDUE]
    
    psllw mm0,15
    
    movq mm3,[2*8+RESIDUE]
    
    movq mm4,[3*8+RESIDUE]
    
    psrlw mm0,8
    
    movq mm5,[4*8+RESIDUE]
    
    movq mm6,[5*8+RESIDUE]
    
    paddsw mm1,mm0
    
    paddsw mm2,mm0
    
    packuswb mm1,mm2
    
    paddsw mm3,mm0
    
    paddsw mm4,mm0
    
    packuswb mm3,mm4
    
    paddsw mm5,mm0
    
    paddsw mm6,mm0
    
    packuswb mm5,mm6
    
    movq [DST],mm1
    
    movq [DST+YSTRIDE],mm3
    
    movq [DST+YSTRIDE*2],mm5
    
    movq mm1,[6*8+RESIDUE]
    
    movq mm2,[7*8+RESIDUE]
    
    movq mm3,[8*8+RESIDUE]
    
    movq mm4,[9*8+RESIDUE]
    
    movq mm5,[10*8+RESIDUE]
    
    movq mm6,[11*8+RESIDUE]
    
    paddsw mm1,mm0
    
    paddsw mm2,mm0
    
    packuswb mm1,mm2
    
    paddsw mm3,mm0
    
    paddsw mm4,mm0
    
    packuswb mm3,mm4
    
    paddsw mm5,mm0
    
    paddsw mm6,mm0
    
    packuswb mm5,mm6
    
    movq [DST+YSTRIDE3],mm1
    
    movq [DST4],mm3
    
    movq [DST4+YSTRIDE],mm5
    
    movq mm1,[12*8+RESIDUE]
    
    movq mm2,[13*8+RESIDUE]
    
    movq mm3,[14*8+RESIDUE]
    
    movq mm4,[15*8+RESIDUE]
    
    paddsw mm1,mm0
    
    paddsw mm2,mm0
    
    packuswb mm1,mm2
    
    paddsw mm3,mm0
    
    paddsw mm4,mm0
    
    packuswb mm3,mm4
    
    movq [DST4+YSTRIDE*2],mm1
    
    movq [DST4+YSTRIDE3],mm3
#undef DST
#undef DST4
#undef YSTRIDE
#undef YSTRIDE3
#undef RESIDUE
  }
}

void oc_frag_recon_inter_mmx(unsigned char *_dst,const unsigned char *_src,
 int _ystride,const ogg_int16_t *_residue){
  int i;
  
  __asm pxor mm0,mm0;
  for(i=4;i-->0;){
    __asm{
#define DST edx
#define SRC ecx
#define YSTRIDE edi
#define RESIDUE eax
      mov DST,_dst
      mov SRC,_src
      mov YSTRIDE,_ystride
      mov RESIDUE,_residue
      
      movq mm3,[SRC]
      
      movq mm7,[SRC+YSTRIDE]
      
      movq mm4,mm3
      
      punpckhbw mm4,mm0
      
      punpcklbw mm3,mm0
      
      paddsw mm4,[8+RESIDUE]
      
      movq mm2,mm7
      
      paddsw  mm3,[RESIDUE]
      
      punpckhbw mm2,mm0
      
      packuswb mm3,mm4
      
      punpcklbw mm7,mm0
      
      paddsw mm7,[16+RESIDUE]
      
      paddsw mm2,[24+RESIDUE]
      
      lea RESIDUE,[32+RESIDUE]
      
      packuswb mm7,mm2
      
      lea SRC,[SRC+YSTRIDE*2]
      
      movq [DST],mm3
      
      movq [DST+YSTRIDE],mm7
      
      lea DST,[DST+YSTRIDE*2]
      mov _residue,RESIDUE
      mov _dst,DST
      mov _src,SRC
#undef DST
#undef SRC
#undef YSTRIDE
#undef RESIDUE
    }
  }
}

void oc_frag_recon_inter2_mmx(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t *_residue){
  int i;
  
  __asm pxor mm7,mm7;
  for(i=4;i-->0;){
    __asm{
#define SRC1 ecx
#define SRC2 edi
#define YSTRIDE esi
#define RESIDUE edx
#define DST eax
      mov YSTRIDE,_ystride
      mov DST,_dst
      mov RESIDUE,_residue
      mov SRC1,_src1
      mov SRC2,_src2
      
      movq mm0,[SRC1]
      
      movq mm2,[SRC2]
      
      movq mm1,mm0
      
      movq mm3,mm2
      
      movq mm4,[SRC1+YSTRIDE]
      
      punpcklbw mm0,mm7
      
      movq mm5,[SRC2+YSTRIDE]
      
      punpckhbw mm1,mm7
      
      punpcklbw mm2,mm7
      
      punpckhbw mm3,mm7
      
      lea SRC1,[SRC1+YSTRIDE*2]
      
      lea SRC2,[SRC2+YSTRIDE*2]
      
      paddsw mm0,mm2
      
      paddsw mm1,mm3
      
      movq mm2,mm4
      
      psraw mm0,1
      
      movq mm3,mm5
      
      punpcklbw mm4,mm7
      
      psraw mm1,1
      
      punpckhbw mm2,mm7
      
      paddsw mm0,[RESIDUE]
      
      punpcklbw mm5,mm7
      
      paddsw mm1,[8+RESIDUE]
      
      punpckhbw mm3,mm7
      
      paddsw mm5,mm4
      
      packuswb mm0,mm1
      
      paddsw mm3,mm2
      
      movq [DST],mm0
      
      psraw mm5,1
      
      psraw mm3,1
      
      paddsw mm5,[16+RESIDUE]
      
      paddsw mm3,[24+RESIDUE]
      
      packuswb  mm5,mm3
      
      movq [DST+YSTRIDE],mm5
      
      add RESIDUE,32
      
      lea DST,[DST+YSTRIDE*2]
      mov _dst,DST
      mov _residue,RESIDUE
      mov _src1,SRC1
      mov _src2,SRC2
#undef SRC1
#undef SRC2
#undef YSTRIDE
#undef RESIDUE
#undef DST
    }
  }
}

void oc_restore_fpu_mmx(void){
  __asm emms;
}

#endif
