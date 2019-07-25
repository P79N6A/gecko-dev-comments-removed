


















#include "x86int.h"
#include "../dct.h"

#if defined(OC_X86_ASM)



#define OC_COSINE_OFFSET (0)

#define OC_EIGHT_OFFSET  (56)




#define OC_IDCT_BEGIN(_y,_x) \
  "#OC_IDCT_BEGIN\n\t" \
  "movq "OC_I(3,_x)",%%mm2\n\t" \
  "movq "OC_MEM_OFFS(0x30,c)",%%mm6\n\t" \
  "movq %%mm2,%%mm4\n\t" \
  "movq "OC_J(5,_x)",%%mm7\n\t" \
  "pmulhw %%mm6,%%mm4\n\t" \
  "movq "OC_MEM_OFFS(0x50,c)",%%mm1\n\t" \
  "pmulhw %%mm7,%%mm6\n\t" \
  "movq %%mm1,%%mm5\n\t" \
  "pmulhw %%mm2,%%mm1\n\t" \
  "movq "OC_I(1,_x)",%%mm3\n\t" \
  "pmulhw %%mm7,%%mm5\n\t" \
  "movq "OC_MEM_OFFS(0x10,c)",%%mm0\n\t" \
  "paddw %%mm2,%%mm4\n\t" \
  "paddw %%mm7,%%mm6\n\t" \
  "paddw %%mm1,%%mm2\n\t" \
  "movq "OC_J(7,_x)",%%mm1\n\t" \
  "paddw %%mm5,%%mm7\n\t" \
  "movq %%mm0,%%mm5\n\t" \
  "pmulhw %%mm3,%%mm0\n\t" \
  "paddw %%mm7,%%mm4\n\t" \
  "pmulhw %%mm1,%%mm5\n\t" \
  "movq "OC_MEM_OFFS(0x70,c)",%%mm7\n\t" \
  "psubw %%mm2,%%mm6\n\t" \
  "paddw %%mm3,%%mm0\n\t" \
  "pmulhw %%mm7,%%mm3\n\t" \
  "movq "OC_I(2,_x)",%%mm2\n\t" \
  "pmulhw %%mm1,%%mm7\n\t" \
  "paddw %%mm1,%%mm5\n\t" \
  "movq %%mm2,%%mm1\n\t" \
  "pmulhw "OC_MEM_OFFS(0x20,c)",%%mm2\n\t" \
  "psubw %%mm5,%%mm3\n\t" \
  "movq "OC_J(6,_x)",%%mm5\n\t" \
  "paddw %%mm7,%%mm0\n\t" \
  "movq %%mm5,%%mm7\n\t" \
  "psubw %%mm4,%%mm0\n\t" \
  "pmulhw "OC_MEM_OFFS(0x20,c)",%%mm5\n\t" \
  "paddw %%mm1,%%mm2\n\t" \
  "pmulhw "OC_MEM_OFFS(0x60,c)",%%mm1\n\t" \
  "paddw %%mm4,%%mm4\n\t" \
  "paddw %%mm0,%%mm4\n\t" \
  "psubw %%mm6,%%mm3\n\t" \
  "paddw %%mm7,%%mm5\n\t" \
  "paddw %%mm6,%%mm6\n\t" \
  "pmulhw "OC_MEM_OFFS(0x60,c)",%%mm7\n\t" \
  "paddw %%mm3,%%mm6\n\t" \
  "movq %%mm4,"OC_I(1,_y)"\n\t" \
  "psubw %%mm5,%%mm1\n\t" \
  "movq "OC_MEM_OFFS(0x40,c)",%%mm4\n\t" \
  "movq %%mm3,%%mm5\n\t" \
  "pmulhw %%mm4,%%mm3\n\t" \
  "paddw %%mm2,%%mm7\n\t" \
  "movq %%mm6,"OC_I(2,_y)"\n\t" \
  "movq %%mm0,%%mm2\n\t" \
  "movq "OC_I(0,_x)",%%mm6\n\t" \
  "pmulhw %%mm4,%%mm0\n\t" \
  "paddw %%mm3,%%mm5\n\t" \
  "movq "OC_J(4,_x)",%%mm3\n\t" \
  "psubw %%mm1,%%mm5\n\t" \
  "paddw %%mm0,%%mm2\n\t" \
  "psubw %%mm3,%%mm6\n\t" \
  "movq %%mm6,%%mm0\n\t" \
  "pmulhw %%mm4,%%mm6\n\t" \
  "paddw %%mm3,%%mm3\n\t" \
  "paddw %%mm1,%%mm1\n\t" \
  "paddw %%mm0,%%mm3\n\t" \
  "paddw %%mm5,%%mm1\n\t" \
  "pmulhw %%mm3,%%mm4\n\t" \
  "paddw %%mm0,%%mm6\n\t" \
  "psubw %%mm2,%%mm6\n\t" \
  "paddw %%mm2,%%mm2\n\t" \
  "movq "OC_I(1,_y)",%%mm0\n\t" \
  "paddw %%mm6,%%mm2\n\t" \
  "paddw %%mm3,%%mm4\n\t" \
  "psubw %%mm1,%%mm2\n\t" \
  "#end OC_IDCT_BEGIN\n\t" \


#define OC_ROW_IDCT(_y,_x) \
  "#OC_ROW_IDCT\n" \
  OC_IDCT_BEGIN(_y,_x) \
  /*r3=D'*/ \
  "movq "OC_I(2,_y)",%%mm3\n\t" \
  /*r4=E'=E-G*/ \
  "psubw %%mm7,%%mm4\n\t" \
  /*r1=H'+H'*/ \
  "paddw %%mm1,%%mm1\n\t" \
  /*r7=G+G*/ \
  "paddw %%mm7,%%mm7\n\t" \
  /*r1=R1=A''+H'*/ \
  "paddw %%mm2,%%mm1\n\t" \
  /*r7=G'=E+G*/ \
  "paddw %%mm4,%%mm7\n\t" \
  /*r4=R4=E'-D'*/ \
  "psubw %%mm3,%%mm4\n\t" \
  "paddw %%mm3,%%mm3\n\t" \
  /*r6=R6=F'-B''*/ \
  "psubw %%mm5,%%mm6\n\t" \
  "paddw %%mm5,%%mm5\n\t" \
  /*r3=R3=E'+D'*/ \
  "paddw %%mm4,%%mm3\n\t" \
  /*r5=R5=F'+B''*/ \
  "paddw %%mm6,%%mm5\n\t" \
  /*r7=R7=G'-C'*/ \
  "psubw %%mm0,%%mm7\n\t" \
  "paddw %%mm0,%%mm0\n\t" \
  /*Save R1.*/ \
  "movq %%mm1,"OC_I(1,_y)"\n\t" \
  /*r0=R0=G.+C.*/ \
  "paddw %%mm7,%%mm0\n\t" \
  "#end OC_ROW_IDCT\n\t" \





























#define OC_TRANSPOSE(_y) \
  "#OC_TRANSPOSE\n\t" \
  "movq %%mm4,%%mm1\n\t" \
  "punpcklwd %%mm5,%%mm4\n\t" \
  "movq %%mm0,"OC_I(0,_y)"\n\t" \
  "punpckhwd %%mm5,%%mm1\n\t" \
  "movq %%mm6,%%mm0\n\t" \
  "punpcklwd %%mm7,%%mm6\n\t" \
  "movq %%mm4,%%mm5\n\t" \
  "punpckldq %%mm6,%%mm4\n\t" \
  "punpckhdq %%mm6,%%mm5\n\t" \
  "movq %%mm1,%%mm6\n\t" \
  "movq %%mm4,"OC_J(4,_y)"\n\t" \
  "punpckhwd %%mm7,%%mm0\n\t" \
  "movq %%mm5,"OC_J(5,_y)"\n\t" \
  "punpckhdq %%mm0,%%mm6\n\t" \
  "movq "OC_I(0,_y)",%%mm4\n\t" \
  "punpckldq %%mm0,%%mm1\n\t" \
  "movq "OC_I(1,_y)",%%mm5\n\t" \
  "movq %%mm4,%%mm0\n\t" \
  "movq %%mm6,"OC_J(7,_y)"\n\t" \
  "punpcklwd %%mm5,%%mm0\n\t" \
  "movq %%mm1,"OC_J(6,_y)"\n\t" \
  "punpckhwd %%mm5,%%mm4\n\t" \
  "movq %%mm2,%%mm5\n\t" \
  "punpcklwd %%mm3,%%mm2\n\t" \
  "movq %%mm0,%%mm1\n\t" \
  "punpckldq %%mm2,%%mm0\n\t" \
  "punpckhdq %%mm2,%%mm1\n\t" \
  "movq %%mm4,%%mm2\n\t" \
  "movq %%mm0,"OC_I(0,_y)"\n\t" \
  "punpckhwd %%mm3,%%mm5\n\t" \
  "movq %%mm1,"OC_I(1,_y)"\n\t" \
  "punpckhdq %%mm5,%%mm4\n\t" \
  "punpckldq %%mm5,%%mm2\n\t" \
  "movq %%mm4,"OC_I(3,_y)"\n\t" \
  "movq %%mm2,"OC_I(2,_y)"\n\t" \
  "#end OC_TRANSPOSE\n\t" \


#define OC_COLUMN_IDCT(_y) \
  "#OC_COLUMN_IDCT\n" \
  OC_IDCT_BEGIN(_y,_y) \
  "paddw "OC_MEM_OFFS(0x00,c)",%%mm2\n\t" \
  /*r1=H'+H'*/ \
  "paddw %%mm1,%%mm1\n\t" \
  /*r1=R1=A''+H'*/ \
  "paddw %%mm2,%%mm1\n\t" \
  /*r2=NR2*/ \
  "psraw $4,%%mm2\n\t" \
  /*r4=E'=E-G*/ \
  "psubw %%mm7,%%mm4\n\t" \
  /*r1=NR1*/ \
  "psraw $4,%%mm1\n\t" \
  /*r3=D'*/ \
  "movq "OC_I(2,_y)",%%mm3\n\t" \
  /*r7=G+G*/ \
  "paddw %%mm7,%%mm7\n\t" \
  /*Store NR2 at I(2).*/ \
  "movq %%mm2,"OC_I(2,_y)"\n\t" \
  /*r7=G'=E+G*/ \
  "paddw %%mm4,%%mm7\n\t" \
  /*Store NR1 at I(1).*/ \
  "movq %%mm1,"OC_I(1,_y)"\n\t" \
  /*r4=R4=E'-D'*/ \
  "psubw %%mm3,%%mm4\n\t" \
  "paddw "OC_MEM_OFFS(0x00,c)",%%mm4\n\t" \
  /*r3=D'+D'*/ \
  "paddw %%mm3,%%mm3\n\t" \
  /*r3=R3=E'+D'*/ \
  "paddw %%mm4,%%mm3\n\t" \
  /*r4=NR4*/ \
  "psraw $4,%%mm4\n\t" \
  /*r6=R6=F'-B''*/ \
  "psubw %%mm5,%%mm6\n\t" \
  /*r3=NR3*/ \
  "psraw $4,%%mm3\n\t" \
  "paddw "OC_MEM_OFFS(0x00,c)",%%mm6\n\t" \
  /*r5=B''+B''*/ \
  "paddw %%mm5,%%mm5\n\t" \
  /*r5=R5=F'+B''*/ \
  "paddw %%mm6,%%mm5\n\t" \
  /*r6=NR6*/ \
  "psraw $4,%%mm6\n\t" \
  /*Store NR4 at J(4).*/ \
  "movq %%mm4,"OC_J(4,_y)"\n\t" \
  /*r5=NR5*/ \
  "psraw $4,%%mm5\n\t" \
  /*Store NR3 at I(3).*/ \
  "movq %%mm3,"OC_I(3,_y)"\n\t" \
  /*r7=R7=G'-C'*/ \
  "psubw %%mm0,%%mm7\n\t" \
  "paddw "OC_MEM_OFFS(0x00,c)",%%mm7\n\t" \
  /*r0=C'+C'*/ \
  "paddw %%mm0,%%mm0\n\t" \
  /*r0=R0=G'+C'*/ \
  "paddw %%mm7,%%mm0\n\t" \
  /*r7=NR7*/ \
  "psraw $4,%%mm7\n\t" \
  /*Store NR6 at J(6).*/ \
  "movq %%mm6,"OC_J(6,_y)"\n\t" \
  /*r0=NR0*/ \
  "psraw $4,%%mm0\n\t" \
  /*Store NR5 at J(5).*/ \
  "movq %%mm5,"OC_J(5,_y)"\n\t" \
  /*Store NR7 at J(7).*/ \
  "movq %%mm7,"OC_J(7,_y)"\n\t" \
  /*Store NR0 at I(0).*/ \
  "movq %%mm0,"OC_I(0,_y)"\n\t" \
  "#end OC_COLUMN_IDCT\n\t" \

static void oc_idct8x8_slow_mmx(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  

  __asm__ __volatile__(
#define OC_I(_k,_y)   OC_MEM_OFFS((_k)*16,_y)
#define OC_J(_k,_y)   OC_MEM_OFFS(((_k)-4)*16+8,_y)
    OC_ROW_IDCT(y,x)
    OC_TRANSPOSE(y)
#undef  OC_I
#undef  OC_J
#define OC_I(_k,_y)   OC_MEM_OFFS((_k)*16+64,_y)
#define OC_J(_k,_y)   OC_MEM_OFFS(((_k)-4)*16+72,_y)
    OC_ROW_IDCT(y,x)
    OC_TRANSPOSE(y)
#undef  OC_I
#undef  OC_J
#define OC_I(_k,_y)   OC_MEM_OFFS((_k)*16,_y)
#define OC_J(_k,_y)   OC_I(_k,_y)
    OC_COLUMN_IDCT(y)
#undef  OC_I
#undef  OC_J
#define OC_I(_k,_y)   OC_MEM_OFFS((_k)*16+8,_y)
#define OC_J(_k,_y)   OC_I(_k,_y)
    OC_COLUMN_IDCT(y)
#undef  OC_I
#undef  OC_J
    :[y]"=m"OC_ARRAY_OPERAND(ogg_int16_t,_y,64)
    :[x]"m"OC_CONST_ARRAY_OPERAND(ogg_int16_t,_x,64),
     [c]"m"OC_CONST_ARRAY_OPERAND(ogg_int16_t,OC_IDCT_CONSTS,128)
  );
  if(_x!=_y){
    int i;
    __asm__ __volatile__("pxor %%mm0,%%mm0\n\t"::);
    for(i=0;i<4;i++){
      __asm__ __volatile__(
        "movq %%mm0,"OC_MEM_OFFS(0x00,x)"\n\t"
        "movq %%mm0,"OC_MEM_OFFS(0x08,x)"\n\t"
        "movq %%mm0,"OC_MEM_OFFS(0x10,x)"\n\t"
        "movq %%mm0,"OC_MEM_OFFS(0x18,x)"\n\t"
        :[x]"=m"OC_ARRAY_OPERAND(ogg_int16_t,_x+16*i,16)
      );
    }
  }
}


#define OC_IDCT_BEGIN_10(_y,_x) \
 "#OC_IDCT_BEGIN_10\n\t" \
 "movq "OC_I(3,_x)",%%mm2\n\t" \
 "nop\n\t" \
 "movq "OC_MEM_OFFS(0x30,c)",%%mm6\n\t" \
 "movq %%mm2,%%mm4\n\t" \
 "movq "OC_MEM_OFFS(0x50,c)",%%mm1\n\t" \
 "pmulhw %%mm6,%%mm4\n\t" \
 "movq "OC_I(1,_x)",%%mm3\n\t" \
 "pmulhw %%mm2,%%mm1\n\t" \
 "movq "OC_MEM_OFFS(0x10,c)",%%mm0\n\t" \
 "paddw %%mm2,%%mm4\n\t" \
 "pxor %%mm6,%%mm6\n\t" \
 "paddw %%mm1,%%mm2\n\t" \
 "movq "OC_I(2,_x)",%%mm5\n\t" \
 "pmulhw %%mm3,%%mm0\n\t" \
 "movq %%mm5,%%mm1\n\t" \
 "paddw %%mm3,%%mm0\n\t" \
 "pmulhw "OC_MEM_OFFS(0x70,c)",%%mm3\n\t" \
 "psubw %%mm2,%%mm6\n\t" \
 "pmulhw "OC_MEM_OFFS(0x20,c)",%%mm5\n\t" \
 "psubw %%mm4,%%mm0\n\t" \
 "movq "OC_I(2,_x)",%%mm7\n\t" \
 "paddw %%mm4,%%mm4\n\t" \
 "paddw %%mm5,%%mm7\n\t" \
 "paddw %%mm0,%%mm4\n\t" \
 "pmulhw "OC_MEM_OFFS(0x60,c)",%%mm1\n\t" \
 "psubw %%mm6,%%mm3\n\t" \
 "movq %%mm4,"OC_I(1,_y)"\n\t" \
 "paddw %%mm6,%%mm6\n\t" \
 "movq "OC_MEM_OFFS(0x40,c)",%%mm4\n\t" \
 "paddw %%mm3,%%mm6\n\t" \
 "movq %%mm3,%%mm5\n\t" \
 "pmulhw %%mm4,%%mm3\n\t" \
 "movq %%mm6,"OC_I(2,_y)"\n\t" \
 "movq %%mm0,%%mm2\n\t" \
 "movq "OC_I(0,_x)",%%mm6\n\t" \
 "pmulhw %%mm4,%%mm0\n\t" \
 "paddw %%mm3,%%mm5\n\t" \
 "paddw %%mm0,%%mm2\n\t" \
 "psubw %%mm1,%%mm5\n\t" \
 "pmulhw %%mm4,%%mm6\n\t" \
 "paddw "OC_I(0,_x)",%%mm6\n\t" \
 "paddw %%mm1,%%mm1\n\t" \
 "movq %%mm6,%%mm4\n\t" \
 "paddw %%mm5,%%mm1\n\t" \
 "psubw %%mm2,%%mm6\n\t" \
 "paddw %%mm2,%%mm2\n\t" \
 "movq "OC_I(1,_y)",%%mm0\n\t" \
 "paddw %%mm6,%%mm2\n\t" \
 "psubw %%mm1,%%mm2\n\t" \
 "nop\n\t" \
 "#end OC_IDCT_BEGIN_10\n\t" \


#define OC_ROW_IDCT_10(_y,_x) \
 "#OC_ROW_IDCT_10\n\t" \
 OC_IDCT_BEGIN_10(_y,_x) \
 /*r3=D'*/ \
 "movq "OC_I(2,_y)",%%mm3\n\t" \
 /*r4=E'=E-G*/ \
 "psubw %%mm7,%%mm4\n\t" \
 /*r1=H'+H'*/ \
 "paddw %%mm1,%%mm1\n\t" \
 /*r7=G+G*/ \
 "paddw %%mm7,%%mm7\n\t" \
 /*r1=R1=A''+H'*/ \
 "paddw %%mm2,%%mm1\n\t" \
 /*r7=G'=E+G*/ \
 "paddw %%mm4,%%mm7\n\t" \
 /*r4=R4=E'-D'*/ \
 "psubw %%mm3,%%mm4\n\t" \
 "paddw %%mm3,%%mm3\n\t" \
 /*r6=R6=F'-B''*/ \
 "psubw %%mm5,%%mm6\n\t" \
 "paddw %%mm5,%%mm5\n\t" \
 /*r3=R3=E'+D'*/ \
 "paddw %%mm4,%%mm3\n\t" \
 /*r5=R5=F'+B''*/ \
 "paddw %%mm6,%%mm5\n\t" \
 /*r7=R7=G'-C'*/ \
 "psubw %%mm0,%%mm7\n\t" \
 "paddw %%mm0,%%mm0\n\t" \
 /*Save R1.*/ \
 "movq %%mm1,"OC_I(1,_y)"\n\t" \
 /*r0=R0=G'+C'*/ \
 "paddw %%mm7,%%mm0\n\t" \
 "#end OC_ROW_IDCT_10\n\t" \


#define OC_COLUMN_IDCT_10(_y) \
 "#OC_COLUMN_IDCT_10\n\t" \
 OC_IDCT_BEGIN_10(_y,_y) \
 "paddw "OC_MEM_OFFS(0x00,c)",%%mm2\n\t" \
 /*r1=H'+H'*/ \
 "paddw %%mm1,%%mm1\n\t" \
 /*r1=R1=A''+H'*/ \
 "paddw %%mm2,%%mm1\n\t" \
 /*r2=NR2*/ \
 "psraw $4,%%mm2\n\t" \
 /*r4=E'=E-G*/ \
 "psubw %%mm7,%%mm4\n\t" \
 /*r1=NR1*/ \
 "psraw $4,%%mm1\n\t" \
 /*r3=D'*/ \
 "movq "OC_I(2,_y)",%%mm3\n\t" \
 /*r7=G+G*/ \
 "paddw %%mm7,%%mm7\n\t" \
 /*Store NR2 at I(2).*/ \
 "movq %%mm2,"OC_I(2,_y)"\n\t" \
 /*r7=G'=E+G*/ \
 "paddw %%mm4,%%mm7\n\t" \
 /*Store NR1 at I(1).*/ \
 "movq %%mm1,"OC_I(1,_y)"\n\t" \
 /*r4=R4=E'-D'*/ \
 "psubw %%mm3,%%mm4\n\t" \
 "paddw "OC_MEM_OFFS(0x00,c)",%%mm4\n\t" \
 /*r3=D'+D'*/ \
 "paddw %%mm3,%%mm3\n\t" \
 /*r3=R3=E'+D'*/ \
 "paddw %%mm4,%%mm3\n\t" \
 /*r4=NR4*/ \
 "psraw $4,%%mm4\n\t" \
 /*r6=R6=F'-B''*/ \
 "psubw %%mm5,%%mm6\n\t" \
 /*r3=NR3*/ \
 "psraw $4,%%mm3\n\t" \
 "paddw "OC_MEM_OFFS(0x00,c)",%%mm6\n\t" \
 /*r5=B''+B''*/ \
 "paddw %%mm5,%%mm5\n\t" \
 /*r5=R5=F'+B''*/ \
 "paddw %%mm6,%%mm5\n\t" \
 /*r6=NR6*/ \
 "psraw $4,%%mm6\n\t" \
 /*Store NR4 at J(4).*/ \
 "movq %%mm4,"OC_J(4,_y)"\n\t" \
 /*r5=NR5*/ \
 "psraw $4,%%mm5\n\t" \
 /*Store NR3 at I(3).*/ \
 "movq %%mm3,"OC_I(3,_y)"\n\t" \
 /*r7=R7=G'-C'*/ \
 "psubw %%mm0,%%mm7\n\t" \
 "paddw "OC_MEM_OFFS(0x00,c)",%%mm7\n\t" \
 /*r0=C'+C'*/ \
 "paddw %%mm0,%%mm0\n\t" \
 /*r0=R0=G'+C'*/ \
 "paddw %%mm7,%%mm0\n\t" \
 /*r7=NR7*/ \
 "psraw $4,%%mm7\n\t" \
 /*Store NR6 at J(6).*/ \
 "movq %%mm6,"OC_J(6,_y)"\n\t" \
 /*r0=NR0*/ \
 "psraw $4,%%mm0\n\t" \
 /*Store NR5 at J(5).*/ \
 "movq %%mm5,"OC_J(5,_y)"\n\t" \
 /*Store NR7 at J(7).*/ \
 "movq %%mm7,"OC_J(7,_y)"\n\t" \
 /*Store NR0 at I(0).*/ \
 "movq %%mm0,"OC_I(0,_y)"\n\t" \
 "#end OC_COLUMN_IDCT_10\n\t" \

static void oc_idct8x8_10_mmx(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  __asm__ __volatile__(
#define OC_I(_k,_y) OC_MEM_OFFS((_k)*16,_y)
#define OC_J(_k,_y) OC_MEM_OFFS(((_k)-4)*16+8,_y)
    

    OC_ROW_IDCT_10(y,x)
    OC_TRANSPOSE(y)
#undef  OC_I
#undef  OC_J
#define OC_I(_k,_y) OC_MEM_OFFS((_k)*16,_y)
#define OC_J(_k,_y) OC_I(_k,_y)
    OC_COLUMN_IDCT_10(y)
#undef  OC_I
#undef  OC_J
#define OC_I(_k,_y) OC_MEM_OFFS((_k)*16+8,_y)
#define OC_J(_k,_y) OC_I(_k,_y)
    OC_COLUMN_IDCT_10(y)
#undef  OC_I
#undef  OC_J
    :[y]"=m"OC_ARRAY_OPERAND(ogg_int16_t,_y,64)
    :[x]"m"OC_CONST_ARRAY_OPERAND(ogg_int16_t,_x,64),
     [c]"m"OC_CONST_ARRAY_OPERAND(ogg_int16_t,OC_IDCT_CONSTS,128)
  );
  if(_x!=_y){
    __asm__ __volatile__(
      "pxor %%mm0,%%mm0\n\t"
      "movq %%mm0,"OC_MEM_OFFS(0x00,x)"\n\t"
      "movq %%mm0,"OC_MEM_OFFS(0x10,x)"\n\t"
      "movq %%mm0,"OC_MEM_OFFS(0x20,x)"\n\t"
      "movq %%mm0,"OC_MEM_OFFS(0x30,x)"\n\t"
      :[x]"+m"OC_ARRAY_OPERAND(ogg_int16_t,_x,28)
    );
  }
}




void oc_idct8x8_mmx(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi){
  























  
  if(_last_zzi<=10)oc_idct8x8_10_mmx(_y,_x);
  else oc_idct8x8_slow_mmx(_y,_x);
}

#endif
