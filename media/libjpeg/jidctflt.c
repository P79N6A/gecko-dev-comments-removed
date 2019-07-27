








































#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"               

#ifdef DCT_FLOAT_SUPPORTED






#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. 
#endif






#define DEQUANTIZE(coef,quantval)  (((FAST_FLOAT) (coef)) * (quantval))






GLOBAL(void)
jpeg_idct_float (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                 JCOEFPTR coef_block,
                 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  FAST_FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  FAST_FLOAT tmp10, tmp11, tmp12, tmp13;
  FAST_FLOAT z5, z10, z11, z12, z13;
  JCOEFPTR inptr;
  FLOAT_MULT_TYPE * quantptr;
  FAST_FLOAT * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = cinfo->sample_range_limit;
  int ctr;
  FAST_FLOAT workspace[DCTSIZE2]; 
  #define _0_125 ((FLOAT_MULT_TYPE)0.125)

  

  inptr = coef_block;
  quantptr = (FLOAT_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; ctr--) {
    








    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
        inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] == 0 &&
        inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] == 0 &&
        inptr[DCTSIZE*7] == 0) {
      
      FAST_FLOAT dcval = DEQUANTIZE(inptr[DCTSIZE*0],
                                    quantptr[DCTSIZE*0] * _0_125);

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      wsptr[DCTSIZE*4] = dcval;
      wsptr[DCTSIZE*5] = dcval;
      wsptr[DCTSIZE*6] = dcval;
      wsptr[DCTSIZE*7] = dcval;

      inptr++;                  
      quantptr++;
      wsptr++;
      continue;
    }

    

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0] * _0_125);
    tmp1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2] * _0_125);
    tmp2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4] * _0_125);
    tmp3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6] * _0_125);

    tmp10 = tmp0 + tmp2;        
    tmp11 = tmp0 - tmp2;

    tmp13 = tmp1 + tmp3;        
    tmp12 = (tmp1 - tmp3) * ((FAST_FLOAT) 1.414213562) - tmp13; 

    tmp0 = tmp10 + tmp13;       
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    

    tmp4 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1] * _0_125);
    tmp5 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3] * _0_125);
    tmp6 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5] * _0_125);
    tmp7 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7] * _0_125);

    z13 = tmp6 + tmp5;          
    z10 = tmp6 - tmp5;
    z11 = tmp4 + tmp7;
    z12 = tmp4 - tmp7;

    tmp7 = z11 + z13;           
    tmp11 = (z11 - z13) * ((FAST_FLOAT) 1.414213562); 

    z5 = (z10 + z12) * ((FAST_FLOAT) 1.847759065); 
    tmp10 = z5 - z12 * ((FAST_FLOAT) 1.082392200); 
    tmp12 = z5 - z10 * ((FAST_FLOAT) 2.613125930); 

    tmp6 = tmp12 - tmp7;        
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 - tmp5;

    wsptr[DCTSIZE*0] = tmp0 + tmp7;
    wsptr[DCTSIZE*7] = tmp0 - tmp7;
    wsptr[DCTSIZE*1] = tmp1 + tmp6;
    wsptr[DCTSIZE*6] = tmp1 - tmp6;
    wsptr[DCTSIZE*2] = tmp2 + tmp5;
    wsptr[DCTSIZE*5] = tmp2 - tmp5;
    wsptr[DCTSIZE*3] = tmp3 + tmp4;
    wsptr[DCTSIZE*4] = tmp3 - tmp4;

    inptr++;                    
    quantptr++;
    wsptr++;
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    outptr = output_buf[ctr] + output_col;
    





    

    
    z5 = wsptr[0] + ((FAST_FLOAT) CENTERJSAMPLE + (FAST_FLOAT) 0.5);
    tmp10 = z5 + wsptr[4];
    tmp11 = z5 - wsptr[4];

    tmp13 = wsptr[2] + wsptr[6];
    tmp12 = (wsptr[2] - wsptr[6]) * ((FAST_FLOAT) 1.414213562) - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    

    z13 = wsptr[5] + wsptr[3];
    z10 = wsptr[5] - wsptr[3];
    z11 = wsptr[1] + wsptr[7];
    z12 = wsptr[1] - wsptr[7];

    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * ((FAST_FLOAT) 1.414213562);

    z5 = (z10 + z12) * ((FAST_FLOAT) 1.847759065); 
    tmp10 = z5 - z12 * ((FAST_FLOAT) 1.082392200); 
    tmp12 = z5 - z10 * ((FAST_FLOAT) 2.613125930); 

    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 - tmp5;

    

    outptr[0] = range_limit[((int) (tmp0 + tmp7)) & RANGE_MASK];
    outptr[7] = range_limit[((int) (tmp0 - tmp7)) & RANGE_MASK];
    outptr[1] = range_limit[((int) (tmp1 + tmp6)) & RANGE_MASK];
    outptr[6] = range_limit[((int) (tmp1 - tmp6)) & RANGE_MASK];
    outptr[2] = range_limit[((int) (tmp2 + tmp5)) & RANGE_MASK];
    outptr[5] = range_limit[((int) (tmp2 - tmp5)) & RANGE_MASK];
    outptr[3] = range_limit[((int) (tmp3 + tmp4)) & RANGE_MASK];
    outptr[4] = range_limit[((int) (tmp3 - tmp4)) & RANGE_MASK];

    wsptr += DCTSIZE;           
  }
}

#endif 
