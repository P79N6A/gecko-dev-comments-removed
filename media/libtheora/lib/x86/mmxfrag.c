





















#include <stddef.h>
#include "x86int.h"
#include "mmxfrag.h"

#if defined(OC_X86_ASM)



void oc_frag_copy_mmx(unsigned char *_dst,
 const unsigned char *_src,int _ystride){
  OC_FRAG_COPY_MMX(_dst,_src,_ystride);
}

void oc_frag_recon_intra_mmx(unsigned char *_dst,int _ystride,
 const ogg_int16_t *_residue){
  __asm__ __volatile__(
    
    "pcmpeqw %%mm0,%%mm0\n\t"
    
    "movq 0*8(%[residue]),%%mm1\n\t"
    
    "movq 1*8(%[residue]),%%mm2\n\t"
    
    "psllw $15,%%mm0\n\t"
    
    "movq 2*8(%[residue]),%%mm3\n\t"
    
    "movq 3*8(%[residue]),%%mm4\n\t"
    
    "psrlw $8,%%mm0\n\t"
    
    "movq 4*8(%[residue]),%%mm5\n\t"
    
    "movq 5*8(%[residue]),%%mm6\n\t"
    
    "paddsw %%mm0,%%mm1\n\t"
    
    "paddsw %%mm0,%%mm2\n\t"
    
    "packuswb %%mm2,%%mm1\n\t"
    
    "paddsw %%mm0,%%mm3\n\t"
    
    "paddsw %%mm0,%%mm4\n\t"
    
    "packuswb %%mm4,%%mm3\n\t"
    
    "paddsw %%mm0,%%mm5\n\t"
    
    "paddsw %%mm0,%%mm6\n\t"
    
    "packuswb %%mm6,%%mm5\n\t"
    
    "movq %%mm1,(%[dst])\n\t"
    
    "movq %%mm3,(%[dst],%[ystride])\n\t"
    
    "movq %%mm5,(%[dst],%[ystride],2)\n\t"
    
    "movq 6*8(%[residue]),%%mm1\n\t"
    
    "movq 7*8(%[residue]),%%mm2\n\t"
    
    "movq 8*8(%[residue]),%%mm3\n\t"
    
    "movq 9*8(%[residue]),%%mm4\n\t"
    
    "movq 10*8(%[residue]),%%mm5\n\t"
    
    "movq 11*8(%[residue]),%%mm6\n\t"
    
    "paddsw %%mm0,%%mm1\n\t"
    
    "paddsw %%mm0,%%mm2\n\t"
    
    "packuswb %%mm2,%%mm1\n\t"
    
    "paddsw %%mm0,%%mm3\n\t"
    
    "paddsw %%mm0,%%mm4\n\t"
    
    "packuswb %%mm4,%%mm3\n\t"
    
    "paddsw %%mm0,%%mm5\n\t"
    
    "paddsw %%mm0,%%mm6\n\t"
    
    "packuswb %%mm6,%%mm5\n\t"
    
    "movq %%mm1,(%[dst],%[ystride3])\n\t"
    
    "movq %%mm3,(%[dst4])\n\t"
    
    "movq %%mm5,(%[dst4],%[ystride])\n\t"
    
    "movq 12*8(%[residue]),%%mm1\n\t"
    
    "movq 13*8(%[residue]),%%mm2\n\t"
    
    "movq 14*8(%[residue]),%%mm3\n\t"
    
    "movq 15*8(%[residue]),%%mm4\n\t"
    
    "paddsw %%mm0,%%mm1\n\t"
    
    "paddsw %%mm0,%%mm2\n\t"
    
    "packuswb %%mm2,%%mm1\n\t"
    
    "paddsw %%mm0,%%mm3\n\t"
    
    "paddsw %%mm0,%%mm4\n\t"
    
    "packuswb %%mm4,%%mm3\n\t"
    
    "movq %%mm1,(%[dst4],%[ystride],2)\n\t"
    
    "movq %%mm3,(%[dst4],%[ystride3])\n\t"
    :
    :[residue]"r"(_residue),
     [dst]"r"(_dst),
     [dst4]"r"(_dst+(_ystride<<2)),
     [ystride]"r"((ptrdiff_t)_ystride),
     [ystride3]"r"((ptrdiff_t)_ystride*3)
    :"memory"
  );
}

void oc_frag_recon_inter_mmx(unsigned char *_dst,const unsigned char *_src,
 int _ystride,const ogg_int16_t *_residue){
  int i;
  
  __asm__ __volatile__("pxor %%mm0,%%mm0\n\t"::);
  for(i=4;i-->0;){
    __asm__ __volatile__(
      
      "movq (%[src]),%%mm3\n\t"
      
      "movq (%[src],%[ystride]),%%mm7\n\t"
      
      "movq %%mm3,%%mm4\n\t"
      
      "punpckhbw %%mm0,%%mm4\n\t"
      
      "punpcklbw %%mm0,%%mm3\n\t"
      
      "paddsw 8(%[residue]),%%mm4\n\t"
      
      "movq %%mm7,%%mm2\n\t"
      
      "paddsw (%[residue]), %%mm3\n\t"
      
      "punpckhbw %%mm0,%%mm2\n\t"
      
      "packuswb %%mm4,%%mm3\n\t"
      
      "punpcklbw %%mm0,%%mm7\n\t"
      
      "paddsw 16(%[residue]),%%mm7\n\t"
      
      "paddsw 24(%[residue]),%%mm2\n\t"
      
      "lea 32(%[residue]),%[residue]\n\t"
      
      "packuswb %%mm2,%%mm7\n\t"
      
      "lea (%[src],%[ystride],2),%[src]\n\t"
      
      "movq %%mm3,(%[dst])\n\t"
      
      "movq %%mm7,(%[dst],%[ystride])\n\t"
      
      "lea (%[dst],%[ystride],2),%[dst]\n\t"
      :[residue]"+r"(_residue),[dst]"+r"(_dst),[src]"+r"(_src)
      :[ystride]"r"((ptrdiff_t)_ystride)
      :"memory"
    );
  }
}

void oc_frag_recon_inter2_mmx(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t *_residue){
  int i;
  
  __asm__ __volatile__("pxor %%mm7,%%mm7\n\t"::);
  for(i=4;i-->0;){
    __asm__ __volatile__(
      
      "movq (%[src1]),%%mm0\n\t"
      
      "movq (%[src2]),%%mm2\n\t"
      
      "movq %%mm0,%%mm1\n\t"
      
      "movq %%mm2,%%mm3\n\t"
      
      "movq (%[src1],%[ystride]),%%mm4\n\t"
      
      "punpcklbw %%mm7,%%mm0\n\t"
      
      "movq (%[src2],%[ystride]),%%mm5\n\t"
      
      "punpckhbw %%mm7,%%mm1\n\t"
      
      "punpcklbw %%mm7,%%mm2\n\t"
      
      "punpckhbw %%mm7,%%mm3\n\t"
      
      "lea (%[src1],%[ystride],2),%[src1]\n\t"
      
      "lea (%[src2],%[ystride],2),%[src2]\n\t"
      
      "paddsw %%mm2,%%mm0\n\t"
      
      "paddsw %%mm3,%%mm1\n\t"
      
      "movq %%mm4,%%mm2\n\t"
      
      "psraw $1,%%mm0\n\t"
      
      "movq %%mm5,%%mm3\n\t"
      
      "punpcklbw %%mm7,%%mm4\n\t"
      
      "psraw $1,%%mm1\n\t"
      
      "punpckhbw %%mm7,%%mm2\n\t"
      
      "paddsw (%[residue]),%%mm0\n\t"
      
      "punpcklbw %%mm7,%%mm5\n\t"
      
      "paddsw 8(%[residue]),%%mm1\n\t"
      
      "punpckhbw %%mm7,%%mm3\n\t"
      
      "paddsw %%mm4,%%mm5\n\t"
      
      "packuswb %%mm1,%%mm0\n\t"
      
      "paddsw %%mm2,%%mm3\n\t"
      
      "movq %%mm0,(%[dst])\n\t"
      
      "psraw $1,%%mm5\n\t"
      
      "psraw $1,%%mm3\n\t"
      
      "paddsw 16(%[residue]),%%mm5\n\t"
      
      "paddsw 24(%[residue]),%%mm3\n\t"
      
      "packuswb  %%mm3,%%mm5\n\t"
      
      "movq %%mm5,(%[dst],%[ystride])\n\t"
      
      "add $32,%[residue]\n\t"
      
      "lea (%[dst],%[ystride],2),%[dst]\n\t"
     :[dst]"+r"(_dst),[residue]"+r"(_residue),
      [src1]"+%r"(_src1),[src2]"+r"(_src2)
     :[ystride]"r"((ptrdiff_t)_ystride)
     :"memory"
    );
  }
}

void oc_restore_fpu_mmx(void){
  __asm__ __volatile__("emms\n\t");
}
#endif
