


















#include "x86int.h"
#include "../dct.h"

#if defined(OC_X86_ASM)



#define OC_COSINE_OFFSET (0)

#define OC_EIGHT_OFFSET  (56)




static const __declspec(align(16))ogg_uint16_t
 OC_IDCT_CONSTS[(7+1)*4]={
  (ogg_uint16_t)OC_C1S7,(ogg_uint16_t)OC_C1S7,
  (ogg_uint16_t)OC_C1S7,(ogg_uint16_t)OC_C1S7,
  (ogg_uint16_t)OC_C2S6,(ogg_uint16_t)OC_C2S6,
  (ogg_uint16_t)OC_C2S6,(ogg_uint16_t)OC_C2S6,
  (ogg_uint16_t)OC_C3S5,(ogg_uint16_t)OC_C3S5,
  (ogg_uint16_t)OC_C3S5,(ogg_uint16_t)OC_C3S5,
  (ogg_uint16_t)OC_C4S4,(ogg_uint16_t)OC_C4S4,
  (ogg_uint16_t)OC_C4S4,(ogg_uint16_t)OC_C4S4,
  (ogg_uint16_t)OC_C5S3,(ogg_uint16_t)OC_C5S3,
  (ogg_uint16_t)OC_C5S3,(ogg_uint16_t)OC_C5S3,
  (ogg_uint16_t)OC_C6S2,(ogg_uint16_t)OC_C6S2,
  (ogg_uint16_t)OC_C6S2,(ogg_uint16_t)OC_C6S2,
  (ogg_uint16_t)OC_C7S1,(ogg_uint16_t)OC_C7S1,
  (ogg_uint16_t)OC_C7S1,(ogg_uint16_t)OC_C7S1,
      8,    8,    8,    8
};


#define OC_IDCT_BEGIN __asm{ \
  __asm movq mm2,OC_I(3) \
  __asm movq mm6,OC_C(3) \
  __asm movq mm4,mm2 \
  __asm movq mm7,OC_J(5) \
  __asm pmulhw mm4,mm6 \
  __asm movq mm1,OC_C(5) \
  __asm pmulhw mm6,mm7 \
  __asm movq mm5,mm1 \
  __asm pmulhw mm1,mm2 \
  __asm movq mm3,OC_I(1) \
  __asm pmulhw mm5,mm7 \
  __asm movq mm0,OC_C(1) \
  __asm paddw mm4,mm2 \
  __asm paddw mm6,mm7 \
  __asm paddw mm2,mm1 \
  __asm movq mm1,OC_J(7) \
  __asm paddw mm7,mm5 \
  __asm movq mm5,mm0 \
  __asm pmulhw mm0,mm3 \
  __asm paddw mm4,mm7 \
  __asm pmulhw mm5,mm1 \
  __asm movq mm7,OC_C(7) \
  __asm psubw mm6,mm2 \
  __asm paddw mm0,mm3 \
  __asm pmulhw mm3,mm7 \
  __asm movq mm2,OC_I(2) \
  __asm pmulhw mm7,mm1 \
  __asm paddw mm5,mm1 \
  __asm movq mm1,mm2 \
  __asm pmulhw mm2,OC_C(2) \
  __asm psubw mm3,mm5 \
  __asm movq mm5,OC_J(6) \
  __asm paddw mm0,mm7 \
  __asm movq mm7,mm5 \
  __asm psubw mm0,mm4 \
  __asm pmulhw mm5,OC_C(2) \
  __asm paddw mm2,mm1 \
  __asm pmulhw mm1,OC_C(6) \
  __asm paddw mm4,mm4 \
  __asm paddw mm4,mm0 \
  __asm psubw mm3,mm6 \
  __asm paddw mm5,mm7 \
  __asm paddw mm6,mm6 \
  __asm pmulhw mm7,OC_C(6) \
  __asm paddw mm6,mm3 \
  __asm movq OC_I(1),mm4 \
  __asm psubw mm1,mm5 \
  __asm movq mm4,OC_C(4) \
  __asm movq mm5,mm3 \
  __asm pmulhw mm3,mm4 \
  __asm paddw mm7,mm2 \
  __asm movq OC_I(2),mm6 \
  __asm movq mm2,mm0 \
  __asm movq mm6,OC_I(0) \
  __asm pmulhw mm0,mm4 \
  __asm paddw mm5,mm3 \
  __asm movq mm3,OC_J(4) \
  __asm psubw mm5,mm1 \
  __asm paddw mm2,mm0 \
  __asm psubw mm6,mm3 \
  __asm movq mm0,mm6 \
  __asm pmulhw mm6,mm4 \
  __asm paddw mm3,mm3 \
  __asm paddw mm1,mm1 \
  __asm paddw mm3,mm0 \
  __asm paddw mm1,mm5 \
  __asm pmulhw mm4,mm3 \
  __asm paddw mm6,mm0 \
  __asm psubw mm6,mm2 \
  __asm paddw mm2,mm2 \
  __asm movq mm0,OC_I(1) \
  __asm paddw mm2,mm6 \
  __asm paddw mm4,mm3 \
  __asm psubw mm2,mm1 \
}


#define OC_ROW_IDCT __asm{ \
  OC_IDCT_BEGIN \
  /*r3=D'*/ \
  __asm  movq mm3,OC_I(2) \
  /*r4=E'=E-G*/ \
  __asm  psubw mm4,mm7 \
  /*r1=H'+H'*/ \
  __asm  paddw mm1,mm1 \
  /*r7=G+G*/ \
  __asm  paddw mm7,mm7 \
  /*r1=R1=A''+H'*/ \
  __asm  paddw mm1,mm2 \
  /*r7=G'=E+G*/ \
  __asm  paddw mm7,mm4 \
  /*r4=R4=E'-D'*/ \
  __asm  psubw mm4,mm3 \
  __asm  paddw mm3,mm3 \
  /*r6=R6=F'-B''*/ \
  __asm  psubw mm6,mm5 \
  __asm  paddw mm5,mm5 \
  /*r3=R3=E'+D'*/ \
  __asm  paddw mm3,mm4 \
  /*r5=R5=F'+B''*/ \
  __asm  paddw mm5,mm6 \
  /*r7=R7=G'-C'*/ \
  __asm  psubw mm7,mm0 \
  __asm  paddw mm0,mm0 \
  /*Save R1.*/ \
  __asm  movq OC_I(1),mm1 \
  /*r0=R0=G.+C.*/ \
  __asm  paddw mm0,mm7 \
}





























#define OC_TRANSPOSE __asm{ \
  __asm movq mm1,mm4 \
  __asm punpcklwd mm4,mm5 \
  __asm movq OC_I(0),mm0 \
  __asm punpckhwd mm1,mm5 \
  __asm movq mm0,mm6 \
  __asm punpcklwd mm6,mm7 \
  __asm movq mm5,mm4 \
  __asm punpckldq mm4,mm6 \
  __asm punpckhdq mm5,mm6 \
  __asm movq mm6,mm1 \
  __asm movq OC_J(4),mm4 \
  __asm punpckhwd mm0,mm7 \
  __asm movq OC_J(5),mm5 \
  __asm punpckhdq mm6,mm0 \
  __asm movq mm4,OC_I(0) \
  __asm punpckldq mm1,mm0 \
  __asm movq mm5,OC_I(1) \
  __asm movq mm0,mm4 \
  __asm movq OC_J(7),mm6 \
  __asm punpcklwd mm0,mm5 \
  __asm movq OC_J(6),mm1 \
  __asm punpckhwd mm4,mm5 \
  __asm movq mm5,mm2 \
  __asm punpcklwd mm2,mm3 \
  __asm movq mm1,mm0 \
  __asm punpckldq mm0,mm2 \
  __asm punpckhdq mm1,mm2 \
  __asm movq mm2,mm4 \
  __asm movq OC_I(0),mm0 \
  __asm punpckhwd mm5,mm3 \
  __asm movq OC_I(1),mm1 \
  __asm punpckhdq mm4,mm5 \
  __asm punpckldq mm2,mm5 \
  __asm movq OC_I(3),mm4 \
  __asm movq OC_I(2),mm2 \
}


#define OC_COLUMN_IDCT __asm{ \
  OC_IDCT_BEGIN \
  __asm paddw mm2,OC_8 \
  /*r1=H'+H'*/ \
  __asm paddw mm1,mm1 \
  /*r1=R1=A''+H'*/ \
  __asm paddw mm1,mm2 \
  /*r2=NR2*/ \
  __asm psraw mm2,4 \
  /*r4=E'=E-G*/ \
  __asm psubw mm4,mm7 \
  /*r1=NR1*/ \
  __asm psraw mm1,4 \
  /*r3=D'*/ \
  __asm movq mm3,OC_I(2) \
  /*r7=G+G*/ \
  __asm paddw mm7,mm7 \
  /*Store NR2 at I(2).*/ \
  __asm movq OC_I(2),mm2 \
  /*r7=G'=E+G*/ \
  __asm paddw mm7,mm4 \
  /*Store NR1 at I(1).*/ \
  __asm movq OC_I(1),mm1 \
  /*r4=R4=E'-D'*/ \
  __asm psubw mm4,mm3 \
  __asm paddw mm4,OC_8 \
  /*r3=D'+D'*/ \
  __asm paddw mm3,mm3 \
  /*r3=R3=E'+D'*/ \
  __asm paddw mm3,mm4 \
  /*r4=NR4*/ \
  __asm psraw mm4,4 \
  /*r6=R6=F'-B''*/ \
  __asm psubw mm6,mm5 \
  /*r3=NR3*/ \
  __asm psraw mm3,4 \
  __asm paddw mm6,OC_8 \
  /*r5=B''+B''*/ \
  __asm paddw mm5,mm5 \
  /*r5=R5=F'+B''*/ \
  __asm paddw mm5,mm6 \
  /*r6=NR6*/ \
  __asm psraw mm6,4 \
  /*Store NR4 at J(4).*/ \
  __asm movq OC_J(4),mm4 \
  /*r5=NR5*/ \
  __asm psraw mm5,4 \
  /*Store NR3 at I(3).*/ \
  __asm movq OC_I(3),mm3 \
  /*r7=R7=G'-C'*/ \
  __asm psubw mm7,mm0 \
  __asm paddw mm7,OC_8 \
  /*r0=C'+C'*/ \
  __asm paddw mm0,mm0 \
  /*r0=R0=G'+C'*/ \
  __asm paddw mm0,mm7 \
  /*r7=NR7*/ \
  __asm psraw mm7,4 \
  /*Store NR6 at J(6).*/ \
  __asm movq OC_J(6),mm6 \
  /*r0=NR0*/ \
  __asm psraw mm0,4 \
  /*Store NR5 at J(5).*/ \
  __asm movq OC_J(5),mm5 \
  /*Store NR7 at J(7).*/ \
  __asm movq OC_J(7),mm7 \
  /*Store NR0 at I(0).*/ \
  __asm movq OC_I(0),mm0 \
}

#define OC_MID(_m,_i) [CONSTS+_m+(_i)*8]
#define OC_C(_i)      OC_MID(OC_COSINE_OFFSET,_i-1)
#define OC_8          OC_MID(OC_EIGHT_OFFSET,0)

static void oc_idct8x8_slow(ogg_int16_t _y[64]){
  

  __asm{
#define CONSTS eax
#define Y edx
    mov CONSTS,offset OC_IDCT_CONSTS
    mov Y,_y
#define OC_I(_k)      [Y+_k*16]
#define OC_J(_k)      [Y+(_k-4)*16+8]
    OC_ROW_IDCT
    OC_TRANSPOSE
#undef  OC_I
#undef  OC_J
#define OC_I(_k)      [Y+(_k*16)+64]
#define OC_J(_k)      [Y+(_k-4)*16+72]
    OC_ROW_IDCT
    OC_TRANSPOSE
#undef  OC_I
#undef  OC_J
#define OC_I(_k)      [Y+_k*16]
#define OC_J(_k)      OC_I(_k)
    OC_COLUMN_IDCT
#undef  OC_I
#undef  OC_J
#define OC_I(_k)      [Y+_k*16+8]
#define OC_J(_k)      OC_I(_k)
    OC_COLUMN_IDCT
#undef  OC_I
#undef  OC_J
#undef  CONSTS
#undef  Y
  }
}


#define OC_IDCT_BEGIN_10 __asm{ \
  __asm movq mm2,OC_I(3) \
  __asm nop \
  __asm movq mm6,OC_C(3) \
  __asm movq mm4,mm2 \
  __asm movq mm1,OC_C(5) \
  __asm pmulhw mm4,mm6 \
  __asm movq mm3,OC_I(1) \
  __asm pmulhw mm1,mm2 \
  __asm movq mm0,OC_C(1) \
  __asm paddw mm4,mm2 \
  __asm pxor mm6,mm6 \
  __asm paddw mm2,mm1 \
  __asm movq mm5,OC_I(2) \
  __asm pmulhw mm0,mm3 \
  __asm movq mm1,mm5 \
  __asm paddw mm0,mm3 \
  __asm pmulhw mm3,OC_C(7) \
  __asm psubw mm6,mm2 \
  __asm pmulhw mm5,OC_C(2) \
  __asm psubw mm0,mm4 \
  __asm movq mm7,OC_I(2) \
  __asm paddw mm4,mm4 \
  __asm paddw mm7,mm5 \
  __asm paddw mm4,mm0 \
  __asm pmulhw mm1,OC_C(6) \
  __asm psubw mm3,mm6 \
  __asm movq OC_I(1),mm4 \
  __asm paddw mm6,mm6 \
  __asm movq mm4,OC_C(4) \
  __asm paddw mm6,mm3 \
  __asm movq mm5,mm3 \
  __asm pmulhw mm3,mm4 \
  __asm movq OC_I(2),mm6 \
  __asm movq mm2,mm0 \
  __asm movq mm6,OC_I(0) \
  __asm pmulhw mm0,mm4 \
  __asm paddw mm5,mm3 \
  __asm paddw mm2,mm0 \
  __asm psubw mm5,mm1 \
  __asm pmulhw mm6,mm4 \
  __asm paddw mm6,OC_I(0) \
  __asm paddw mm1,mm1 \
  __asm movq mm4,mm6 \
  __asm paddw mm1,mm5 \
  __asm psubw mm6,mm2 \
  __asm paddw mm2,mm2 \
  __asm movq mm0,OC_I(1) \
  __asm paddw mm2,mm6 \
  __asm psubw mm2,mm1 \
  __asm nop \
}


#define OC_ROW_IDCT_10 __asm{ \
  OC_IDCT_BEGIN_10 \
  /*r3=D'*/ \
   __asm movq mm3,OC_I(2) \
  /*r4=E'=E-G*/ \
   __asm psubw mm4,mm7 \
  /*r1=H'+H'*/ \
   __asm paddw mm1,mm1 \
  /*r7=G+G*/ \
   __asm paddw mm7,mm7 \
  /*r1=R1=A''+H'*/ \
   __asm paddw mm1,mm2 \
  /*r7=G'=E+G*/ \
   __asm paddw mm7,mm4 \
  /*r4=R4=E'-D'*/ \
   __asm psubw mm4,mm3 \
   __asm paddw mm3,mm3 \
  /*r6=R6=F'-B''*/ \
   __asm psubw mm6,mm5 \
   __asm paddw mm5,mm5 \
  /*r3=R3=E'+D'*/ \
   __asm paddw mm3,mm4 \
  /*r5=R5=F'+B''*/ \
   __asm paddw mm5,mm6 \
  /*r7=R7=G'-C'*/ \
   __asm psubw mm7,mm0 \
   __asm paddw mm0,mm0 \
  /*Save R1.*/ \
   __asm movq OC_I(1),mm1 \
  /*r0=R0=G'+C'*/ \
   __asm paddw mm0,mm7 \
}


#define OC_COLUMN_IDCT_10 __asm{ \
  OC_IDCT_BEGIN_10 \
  __asm paddw mm2,OC_8 \
  /*r1=H'+H'*/ \
  __asm paddw mm1,mm1 \
  /*r1=R1=A''+H'*/ \
  __asm paddw mm1,mm2 \
  /*r2=NR2*/ \
  __asm psraw mm2,4 \
  /*r4=E'=E-G*/ \
  __asm psubw mm4,mm7 \
  /*r1=NR1*/ \
  __asm psraw mm1,4 \
  /*r3=D'*/ \
  __asm movq mm3,OC_I(2) \
  /*r7=G+G*/ \
  __asm paddw mm7,mm7 \
  /*Store NR2 at I(2).*/ \
  __asm movq OC_I(2),mm2 \
  /*r7=G'=E+G*/ \
  __asm paddw mm7,mm4 \
  /*Store NR1 at I(1).*/ \
  __asm movq OC_I(1),mm1 \
  /*r4=R4=E'-D'*/ \
  __asm psubw mm4,mm3 \
  __asm paddw mm4,OC_8 \
  /*r3=D'+D'*/ \
  __asm paddw mm3,mm3 \
  /*r3=R3=E'+D'*/ \
  __asm paddw mm3,mm4 \
  /*r4=NR4*/ \
  __asm psraw mm4,4 \
  /*r6=R6=F'-B''*/ \
  __asm psubw mm6,mm5 \
  /*r3=NR3*/ \
  __asm psraw mm3,4 \
  __asm paddw mm6,OC_8 \
  /*r5=B''+B''*/ \
  __asm paddw mm5,mm5 \
  /*r5=R5=F'+B''*/ \
  __asm paddw mm5,mm6 \
  /*r6=NR6*/ \
  __asm psraw mm6,4 \
  /*Store NR4 at J(4).*/ \
  __asm movq OC_J(4),mm4 \
  /*r5=NR5*/ \
  __asm psraw mm5,4 \
  /*Store NR3 at I(3).*/ \
  __asm movq OC_I(3),mm3 \
  /*r7=R7=G'-C'*/ \
  __asm psubw mm7,mm0 \
  __asm paddw mm7,OC_8 \
  /*r0=C'+C'*/ \
  __asm paddw mm0,mm0 \
  /*r0=R0=G'+C'*/ \
  __asm paddw mm0,mm7 \
  /*r7=NR7*/ \
  __asm psraw mm7,4 \
  /*Store NR6 at J(6).*/ \
  __asm movq OC_J(6),mm6 \
  /*r0=NR0*/ \
  __asm psraw mm0,4 \
  /*Store NR5 at J(5).*/ \
  __asm movq OC_J(5),mm5 \
  /*Store NR7 at J(7).*/ \
  __asm movq OC_J(7),mm7 \
  /*Store NR0 at I(0).*/ \
  __asm movq OC_I(0),mm0 \
}

static void oc_idct8x8_10(ogg_int16_t _y[64]){
  __asm{
#define CONSTS eax
#define Y edx
    mov CONSTS,offset OC_IDCT_CONSTS
    mov Y,_y
#define OC_I(_k) [Y+_k*16]
#define OC_J(_k) [Y+(_k-4)*16+8]
    

    OC_ROW_IDCT_10
    OC_TRANSPOSE
#undef  OC_I
#undef  OC_J
#define OC_I(_k) [Y+_k*16]
#define OC_J(_k) OC_I(_k)
    OC_COLUMN_IDCT_10
#undef  OC_I
#undef  OC_J
#define OC_I(_k) [Y+_k*16+8]
#define OC_J(_k) OC_I(_k)
    OC_COLUMN_IDCT_10
#undef  OC_I
#undef  OC_J
#undef  CONSTS
#undef  Y
  }
}




void oc_idct8x8_mmx(ogg_int16_t _y[64],int _last_zzi){
  























  
  if(_last_zzi<10)oc_idct8x8_10(_y);
  else oc_idct8x8_slow(_y);
}

#endif
