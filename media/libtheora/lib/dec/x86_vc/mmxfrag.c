















#include "../../internal.h"








#if defined(USE_ASM)

void oc_frag_recon_intra_mmx(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t *_residue){
  int _save_ebx;
  




  _asm{
    mov       [_save_ebx], ebx
    mov       edi, [_residue]     
    mov       eax, 0x00800080     
    mov       ebx, [_dst_ystride] 
    mov       edx, [_dst]         

    

    movd      mm0, eax            
    movq      mm1, [edi+ 8*0]     
    movq      mm2, [edi+ 8*1]     
    punpckldq mm0, mm0            
    movq      mm3, [edi+ 8*2]     
    movq      mm4, [edi+ 8*3]     
    movq      mm5, [edi+ 8*4]     
    movq      mm6, [edi+ 8*5]     
    paddsw    mm1, mm0            
    paddsw    mm2, mm0            
    packuswb  mm1, mm2            
    paddsw    mm3, mm0            
    paddsw    mm4, mm0            
    packuswb  mm3, mm4            
    paddsw    mm5, mm0            
    paddsw    mm6, mm0            
    packuswb  mm5, mm6            
    movq      [edx], mm1          
    movq      [edx + ebx], mm3    
    movq      [edx + ebx*2], mm5  
    movq      mm1, [edi+ 8*6]     
    lea       ecx, [ebx + ebx*2]  
    movq      mm2, [edi+ 8*7]     
    movq      mm3, [edi+ 8*8]     
    lea       esi, [ebx*4 + ebx]  
    movq      mm4, [edi+ 8*9]     
    movq      mm5, [edi+ 8*10]    
    lea       eax, [ecx*2 + ebx]  
    movq      mm6, [edi+ 8*11]    
    paddsw    mm1, mm0            
    paddsw    mm2, mm0            
    packuswb  mm1, mm2            
    paddsw    mm3, mm0            
    paddsw    mm4, mm0            
    packuswb  mm3, mm4            
    paddsw    mm5, mm0            
    paddsw    mm6, mm0            
    packuswb  mm5, mm6            
    movq      [edx + ecx], mm1    
    movq      [edx + ebx*4], mm3  
    movq      [edx + esi], mm5    
    movq      mm1, [edi+ 8*12]    
    movq      mm2, [edi+ 8*13]    
    movq      mm3, [edi+ 8*14]    
    movq      mm4, [edi+ 8*15]    
    paddsw    mm1, mm0            
    paddsw    mm2, mm0            
    packuswb  mm1, mm2            
    paddsw    mm3, mm0            
    paddsw    mm4, mm0            
    packuswb  mm3, mm4            
    movq      [edx + ecx*2], mm1  
    movq      [edx + eax], mm3    
    mov       ebx, [_save_ebx]
  }
}



void oc_frag_recon_inter_mmx (unsigned char *_dst, int _dst_ystride,
 const unsigned char *_src, int _src_ystride, const ogg_int16_t *_residue){
  int _save_ebx;
  





  _asm{
    mov       [_save_ebx], ebx
    pxor      mm0, mm0          
    mov       esi, [_src]
    mov       edi, [_residue]
    mov       eax, [_src_ystride]
    mov       edx, [_dst]
    mov       ebx, [_dst_ystride]
    mov       ecx, 4

    align 16

nextchunk:
    movq      mm3, [esi]        
    movq      mm1, [edi+0]      
    movq      mm2, [edi+8]      
    movq      mm7, [esi+eax]    
    movq      mm4, mm3          
    movq      mm5, [edi+16]     
    punpckhbw mm4, mm0          
    movq      mm6, [edi+24]     
    punpcklbw mm3, mm0          
    paddsw    mm4, mm2          
    movq      mm2, mm7          
    paddsw    mm3, mm1          
    punpckhbw mm2, mm0          
    packuswb  mm3, mm4          
    punpcklbw mm7, mm0          
    movq      [edx], mm3        
    paddsw    mm2, mm6          
    add       edi, 32           
    paddsw    mm7, mm5          
    sub       ecx, 1            
    packuswb  mm7, mm2          
    lea       esi, [esi+eax*2]  
    movq      [edx + ebx], mm7  
    lea       edx, [edx+ebx*2]  
    jne       nextchunk
    mov       ebx, [_save_ebx]
  }
}


void oc_frag_recon_inter2_mmx(unsigned char *_dst,  int _dst_ystride,
 const unsigned char *_src1,  int _src1_ystride, const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue){
  int _save_ebx;
  














 _asm{
   mov        [_save_ebx], ebx
   mov        eax, 0xfefefefe
   mov        esi, [_src1]
   mov        edi, [_src2]
   movd       mm1, eax
   mov        ebx, [_residue]
   mov        edx, [_dst]
   mov        eax, [_dst_ystride]
   punpckldq  mm1, mm1            
   mov        ecx, 8              
   pxor       mm0, mm0            
   sub        edx, eax            

   align      16

nextrow:
   movq       mm2,  [esi]         
   movq       mm3,  [edi]         
   movq       mm5,  [ebx + 0]     
   movq       mm6,  [ebx + 8]     
   add        esi,  _src1_ystride 
   add        edi,  _src2_ystride 
   movq       mm4,  mm2           
   pand       mm2,  mm3           
   pxor       mm3,  mm4           
   add        ebx,  16            
   pand       mm3,  mm1           
   psrlq      mm3,  1             
   paddd      mm3,  mm2           
   add        edx,  eax           
   movq       mm2,  mm3           
   punpckhbw  mm3,  mm0           
   punpcklbw  mm2,  mm0           
   paddsw     mm3,  mm6           
   paddsw     mm2,  mm5           
   sub        ecx,  1             
   packuswb   mm2,  mm3           
   movq       [edx], mm2          
   jne        nextrow
   mov        ebx, [_save_ebx]
 }
}

void oc_restore_fpu_mmx(void){
  _asm { emms }
}

#endif
