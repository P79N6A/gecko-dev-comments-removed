































#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

#ifdef DCT_IFAST_SUPPORTED






#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. 
#endif




















#define CONST_BITS  8









#if CONST_BITS == 8
#define FIX_0_382683433  ((INT32)   98)		/* FIX(0.382683433) */
#define FIX_0_541196100  ((INT32)  139)		/* FIX(0.541196100) */
#define FIX_0_707106781  ((INT32)  181)		/* FIX(0.707106781) */
#define FIX_1_306562965  ((INT32)  334)		/* FIX(1.306562965) */
#else
#define FIX_0_382683433  FIX(0.382683433)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_707106781  FIX(0.707106781)
#define FIX_1_306562965  FIX(1.306562965)
#endif







#ifndef USE_ACCURATE_ROUNDING
#undef DESCALE
#define DESCALE(x,n)  RIGHT_SHIFT(x, n)
#endif






#define MULTIPLY(var,const)  ((DCTELEM) DESCALE((var) * (const), CONST_BITS))






GLOBAL(void)
jpeg_fdct_ifast (DCTELEM * data)
{
  DCTELEM tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  DCTELEM tmp10, tmp11, tmp12, tmp13;
  DCTELEM z1, z2, z3, z4, z5, z11, z13;
  DCTELEM *dataptr;
  int ctr;
  SHIFT_TEMPS

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    tmp0 = dataptr[0] + dataptr[7];
    tmp7 = dataptr[0] - dataptr[7];
    tmp1 = dataptr[1] + dataptr[6];
    tmp6 = dataptr[1] - dataptr[6];
    tmp2 = dataptr[2] + dataptr[5];
    tmp5 = dataptr[2] - dataptr[5];
    tmp3 = dataptr[3] + dataptr[4];
    tmp4 = dataptr[3] - dataptr[4];
    
    
    
    tmp10 = tmp0 + tmp3;	
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    
    dataptr[0] = tmp10 + tmp11; 
    dataptr[4] = tmp10 - tmp11;
    
    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); 
    dataptr[2] = tmp13 + z1;	
    dataptr[6] = tmp13 - z1;
    
    

    tmp10 = tmp4 + tmp5;	
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    
    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); 
    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; 
    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; 
    z3 = MULTIPLY(tmp11, FIX_0_707106781); 

    z11 = tmp7 + z3;		
    z13 = tmp7 - z3;

    dataptr[5] = z13 + z2;	
    dataptr[3] = z13 - z2;
    dataptr[1] = z11 + z4;
    dataptr[7] = z11 - z4;

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
    tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
    tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
    tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];
    
    
    
    tmp10 = tmp0 + tmp3;	
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    
    dataptr[DCTSIZE*0] = tmp10 + tmp11; 
    dataptr[DCTSIZE*4] = tmp10 - tmp11;
    
    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); 
    dataptr[DCTSIZE*2] = tmp13 + z1; 
    dataptr[DCTSIZE*6] = tmp13 - z1;
    
    

    tmp10 = tmp4 + tmp5;	
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    
    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); 
    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; 
    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; 
    z3 = MULTIPLY(tmp11, FIX_0_707106781); 

    z11 = tmp7 + z3;		
    z13 = tmp7 - z3;

    dataptr[DCTSIZE*5] = z13 + z2; 
    dataptr[DCTSIZE*3] = z13 - z2;
    dataptr[DCTSIZE*1] = z11 + z4;
    dataptr[DCTSIZE*7] = z11 - z4;

    dataptr++;			
  }
}

#endif 
