





















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

#ifdef IDCT_SCALING_SUPPORTED






#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. 
#endif




#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  13
#define PASS1_BITS  2
#else
#define CONST_BITS  13
#define PASS1_BITS  1		/* lose a little precision to avoid overflow */
#endif








#if CONST_BITS == 13
#define FIX_0_211164243  ((INT32)  1730)	/* FIX(0.211164243) */
#define FIX_0_509795579  ((INT32)  4176)	/* FIX(0.509795579) */
#define FIX_0_601344887  ((INT32)  4926)	/* FIX(0.601344887) */
#define FIX_0_720959822  ((INT32)  5906)	/* FIX(0.720959822) */
#define FIX_0_765366865  ((INT32)  6270)	/* FIX(0.765366865) */
#define FIX_0_850430095  ((INT32)  6967)	/* FIX(0.850430095) */
#define FIX_0_899976223  ((INT32)  7373)	/* FIX(0.899976223) */
#define FIX_1_061594337  ((INT32)  8697)	/* FIX(1.061594337) */
#define FIX_1_272758580  ((INT32)  10426)	/* FIX(1.272758580) */
#define FIX_1_451774981  ((INT32)  11893)	/* FIX(1.451774981) */
#define FIX_1_847759065  ((INT32)  15137)	/* FIX(1.847759065) */
#define FIX_2_172734803  ((INT32)  17799)	/* FIX(2.172734803) */
#define FIX_2_562915447  ((INT32)  20995)	/* FIX(2.562915447) */
#define FIX_3_624509785  ((INT32)  29692)	/* FIX(3.624509785) */
#else
#define FIX_0_211164243  FIX(0.211164243)
#define FIX_0_509795579  FIX(0.509795579)
#define FIX_0_601344887  FIX(0.601344887)
#define FIX_0_720959822  FIX(0.720959822)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_850430095  FIX(0.850430095)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_061594337  FIX(1.061594337)
#define FIX_1_272758580  FIX(1.272758580)
#define FIX_1_451774981  FIX(1.451774981)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_2_172734803  FIX(2.172734803)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_624509785  FIX(3.624509785)
#endif









#if BITS_IN_JSAMPLE == 8
#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY(var,const)  ((var) * (const))
#endif







#define DEQUANTIZE(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))







GLOBAL(void)
jpeg_idct_4x4 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp2, tmp10, tmp12;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE*4];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--) {
    
    if (ctr == DCTSIZE-4)
      continue;
    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
	inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*5] == 0 &&
	inptr[DCTSIZE*6] == 0 && inptr[DCTSIZE*7] == 0) {
      
      int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;
      
      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      
      continue;
    }
    
    
    
    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= (CONST_BITS+1);
    
    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp2 = MULTIPLY(z2, FIX_1_847759065) + MULTIPLY(z3, - FIX_0_765366865);
    
    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;
    
    
    
    z1 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    
    tmp0 = MULTIPLY(z1, - FIX_0_211164243) 
	 + MULTIPLY(z2, FIX_1_451774981) 
	 + MULTIPLY(z3, - FIX_2_172734803) 
	 + MULTIPLY(z4, FIX_1_061594337); 
    
    tmp2 = MULTIPLY(z1, - FIX_0_509795579) 
	 + MULTIPLY(z2, - FIX_0_601344887) 
	 + MULTIPLY(z3, FIX_0_899976223) 
	 + MULTIPLY(z4, FIX_2_562915447); 

    
    
    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp2, CONST_BITS-PASS1_BITS+1);
    wsptr[DCTSIZE*3] = (int) DESCALE(tmp10 - tmp2, CONST_BITS-PASS1_BITS+1);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp12 + tmp0, CONST_BITS-PASS1_BITS+1);
    wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 - tmp0, CONST_BITS-PASS1_BITS+1);
  }
  
  

  wsptr = workspace;
  for (ctr = 0; ctr < 4; ctr++) {
    outptr = output_buf[ctr] + output_col;
    

#ifndef NO_ZERO_ROW_TEST
    if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 &&
	wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0) {
      
      JSAMPLE dcval = range_limit[(int) DESCALE((INT32) wsptr[0], PASS1_BITS+3)
				  & RANGE_MASK];
      
      outptr[0] = dcval;
      outptr[1] = dcval;
      outptr[2] = dcval;
      outptr[3] = dcval;
      
      wsptr += DCTSIZE;		
      continue;
    }
#endif
    
    
    
    tmp0 = ((INT32) wsptr[0]) << (CONST_BITS+1);
    
    tmp2 = MULTIPLY((INT32) wsptr[2], FIX_1_847759065)
	 + MULTIPLY((INT32) wsptr[6], - FIX_0_765366865);
    
    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;
    
    
    
    z1 = (INT32) wsptr[7];
    z2 = (INT32) wsptr[5];
    z3 = (INT32) wsptr[3];
    z4 = (INT32) wsptr[1];
    
    tmp0 = MULTIPLY(z1, - FIX_0_211164243) 
	 + MULTIPLY(z2, FIX_1_451774981) 
	 + MULTIPLY(z3, - FIX_2_172734803) 
	 + MULTIPLY(z4, FIX_1_061594337); 
    
    tmp2 = MULTIPLY(z1, - FIX_0_509795579) 
	 + MULTIPLY(z2, - FIX_0_601344887) 
	 + MULTIPLY(z3, FIX_0_899976223) 
	 + MULTIPLY(z4, FIX_2_562915447); 

    
    
    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp2,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE(tmp10 - tmp2,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp12 + tmp0,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE(tmp12 - tmp0,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    
    wsptr += DCTSIZE;		
  }
}







GLOBAL(void)
jpeg_idct_2x2 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp10, z1;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE*2];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--) {
    
    if (ctr == DCTSIZE-2 || ctr == DCTSIZE-4 || ctr == DCTSIZE-6)
      continue;
    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*3] == 0 &&
	inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*7] == 0) {
      
      int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;
      
      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      
      continue;
    }
    
    
    
    z1 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp10 = z1 << (CONST_BITS+2);
    
    

    z1 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    tmp0 = MULTIPLY(z1, - FIX_0_720959822); 
    z1 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp0 += MULTIPLY(z1, FIX_0_850430095); 
    z1 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp0 += MULTIPLY(z1, - FIX_1_272758580); 
    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    tmp0 += MULTIPLY(z1, FIX_3_624509785); 

    
    
    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp0, CONST_BITS-PASS1_BITS+2);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp10 - tmp0, CONST_BITS-PASS1_BITS+2);
  }
  
  

  wsptr = workspace;
  for (ctr = 0; ctr < 2; ctr++) {
    outptr = output_buf[ctr] + output_col;
    

#ifndef NO_ZERO_ROW_TEST
    if (wsptr[1] == 0 && wsptr[3] == 0 && wsptr[5] == 0 && wsptr[7] == 0) {
      
      JSAMPLE dcval = range_limit[(int) DESCALE((INT32) wsptr[0], PASS1_BITS+3)
				  & RANGE_MASK];
      
      outptr[0] = dcval;
      outptr[1] = dcval;
      
      wsptr += DCTSIZE;		
      continue;
    }
#endif
    
    
    
    tmp10 = ((INT32) wsptr[0]) << (CONST_BITS+2);
    
    

    tmp0 = MULTIPLY((INT32) wsptr[7], - FIX_0_720959822) 
	 + MULTIPLY((INT32) wsptr[5], FIX_0_850430095) 
	 + MULTIPLY((INT32) wsptr[3], - FIX_1_272758580) 
	 + MULTIPLY((INT32) wsptr[1], FIX_3_624509785); 

    
    
    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp0,
					  CONST_BITS+PASS1_BITS+3+2)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp10 - tmp0,
					  CONST_BITS+PASS1_BITS+3+2)
			    & RANGE_MASK];
    
    wsptr += DCTSIZE;		
  }
}







GLOBAL(void)
jpeg_idct_1x1 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  int dcval;
  ISLOW_MULT_TYPE * quantptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  SHIFT_TEMPS

  


  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  dcval = DEQUANTIZE(coef_block[0], quantptr[0]);
  dcval = (int) DESCALE((INT32) dcval, 3);

  output_buf[0][output_col] = range_limit[dcval & RANGE_MASK];
}

#endif 
