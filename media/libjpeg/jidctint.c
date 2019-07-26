
















































#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

#ifdef DCT_ISLOW_SUPPORTED






#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCT blocks. 
#endif


































#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  13
#define PASS1_BITS  2
#else
#define CONST_BITS  13
#define PASS1_BITS  1		/* lose a little precision to avoid overflow */
#endif








#if CONST_BITS == 13
#define FIX_0_298631336  ((INT32)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((INT32)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((INT32)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((INT32)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((INT32)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((INT32)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((INT32)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((INT32)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((INT32)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((INT32)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((INT32)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((INT32)  25172)	/* FIX(3.072711026) */
#else
#define FIX_0_298631336  FIX(0.298631336)
#define FIX_0_390180644  FIX(0.390180644)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_175875602  FIX(1.175875602)
#define FIX_1_501321110  FIX(1.501321110)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_1_961570560  FIX(1.961570560)
#define FIX_2_053119869  FIX(2.053119869)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_072711026  FIX(3.072711026)
#endif









#if BITS_IN_JSAMPLE == 8
#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY(var,const)  ((var) * (const))
#endif







#define DEQUANTIZE(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))






GLOBAL(void)
jpeg_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1, z2, z3, z4, z5;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE2];	
  SHIFT_TEMPS

  
  
  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; ctr--) {
    







    
    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
	inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] == 0 &&
	inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] == 0 &&
	inptr[DCTSIZE*7] == 0) {
      
      int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;
      
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
    
    
    
    
    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);
    
    z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);
    
    z2 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);

    tmp0 = (z2 + z3) << CONST_BITS;
    tmp1 = (z2 - z3) << CONST_BITS;
    
    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    
    


    
    tmp0 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    tmp1 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp3 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    
    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602); 
    
    tmp0 = MULTIPLY(tmp0, FIX_0_298631336); 
    tmp1 = MULTIPLY(tmp1, FIX_2_053119869); 
    tmp2 = MULTIPLY(tmp2, FIX_3_072711026); 
    tmp3 = MULTIPLY(tmp3, FIX_1_501321110); 
    z1 = MULTIPLY(z1, - FIX_0_899976223); 
    z2 = MULTIPLY(z2, - FIX_2_562915447); 
    z3 = MULTIPLY(z3, - FIX_1_961570560); 
    z4 = MULTIPLY(z4, - FIX_0_390180644); 
    
    z3 += z5;
    z4 += z5;
    
    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;
    
    
    
    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*7] = (int) DESCALE(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*6] = (int) DESCALE(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*5] = (int) DESCALE(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*3] = (int) DESCALE(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*4] = (int) DESCALE(tmp13 - tmp0, CONST_BITS-PASS1_BITS);
    
    inptr++;			
    quantptr++;
    wsptr++;
  }
  
  
  
  

  wsptr = workspace;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    outptr = output_buf[ctr] + output_col;
    






    
#ifndef NO_ZERO_ROW_TEST
    if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 && wsptr[4] == 0 &&
	wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0) {
      
      JSAMPLE dcval = range_limit[(int) DESCALE((INT32) wsptr[0], PASS1_BITS+3)
				  & RANGE_MASK];
      
      outptr[0] = dcval;
      outptr[1] = dcval;
      outptr[2] = dcval;
      outptr[3] = dcval;
      outptr[4] = dcval;
      outptr[5] = dcval;
      outptr[6] = dcval;
      outptr[7] = dcval;

      wsptr += DCTSIZE;		
      continue;
    }
#endif
    
    
    
    
    z2 = (INT32) wsptr[2];
    z3 = (INT32) wsptr[6];
    
    z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);
    
    tmp0 = ((INT32) wsptr[0] + (INT32) wsptr[4]) << CONST_BITS;
    tmp1 = ((INT32) wsptr[0] - (INT32) wsptr[4]) << CONST_BITS;
    
    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    
    


    
    tmp0 = (INT32) wsptr[7];
    tmp1 = (INT32) wsptr[5];
    tmp2 = (INT32) wsptr[3];
    tmp3 = (INT32) wsptr[1];
    
    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602); 
    
    tmp0 = MULTIPLY(tmp0, FIX_0_298631336); 
    tmp1 = MULTIPLY(tmp1, FIX_2_053119869); 
    tmp2 = MULTIPLY(tmp2, FIX_3_072711026); 
    tmp3 = MULTIPLY(tmp3, FIX_1_501321110); 
    z1 = MULTIPLY(z1, - FIX_0_899976223); 
    z2 = MULTIPLY(z2, - FIX_2_562915447); 
    z3 = MULTIPLY(z3, - FIX_1_961570560); 
    z4 = MULTIPLY(z4, - FIX_0_390180644); 
    
    z3 += z5;
    z4 += z5;
    
    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;
    
    
    
    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp3,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[7] = range_limit[(int) DESCALE(tmp10 - tmp3,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp11 + tmp2,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[6] = range_limit[(int) DESCALE(tmp11 - tmp2,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE(tmp12 + tmp1,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) DESCALE(tmp12 - tmp1,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE(tmp13 + tmp0,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) DESCALE(tmp13 - tmp0,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    
    wsptr += DCTSIZE;		
  }
}

#ifdef IDCT_SCALING_SUPPORTED










GLOBAL(void)
jpeg_idct_7x7 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp2, tmp10, tmp11, tmp12, tmp13;
  INT32 z1, z2, z3;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[7*7];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 7; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp13 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp13 <<= CONST_BITS;
    
    tmp13 += ONE << (CONST_BITS-PASS1_BITS-1);

    z1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp10 = MULTIPLY(z2 - z3, FIX(0.881747734));     
    tmp12 = MULTIPLY(z1 - z2, FIX(0.314692123));     
    tmp11 = tmp10 + tmp12 + tmp13 - MULTIPLY(z2, FIX(1.841218003)); 
    tmp0 = z1 + z3;
    z2 -= tmp0;
    tmp0 = MULTIPLY(tmp0, FIX(1.274162392)) + tmp13; 
    tmp10 += tmp0 - MULTIPLY(z3, FIX(0.077722536));  
    tmp12 += tmp0 - MULTIPLY(z1, FIX(2.470602249));  
    tmp13 += MULTIPLY(z2, FIX(1.414213562));         

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);

    tmp1 = MULTIPLY(z1 + z2, FIX(0.935414347));      
    tmp2 = MULTIPLY(z1 - z2, FIX(0.170262339));      
    tmp0 = tmp1 - tmp2;
    tmp1 += tmp2;
    tmp2 = MULTIPLY(z2 + z3, - FIX(1.378756276));    
    tmp1 += tmp2;
    z2 = MULTIPLY(z1 + z3, FIX(0.613604268));        
    tmp0 += z2;
    tmp2 += z2 + MULTIPLY(z3, FIX(1.870828693));     

    

    wsptr[7*0] = (int) RIGHT_SHIFT(tmp10 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[7*6] = (int) RIGHT_SHIFT(tmp10 - tmp0, CONST_BITS-PASS1_BITS);
    wsptr[7*1] = (int) RIGHT_SHIFT(tmp11 + tmp1, CONST_BITS-PASS1_BITS);
    wsptr[7*5] = (int) RIGHT_SHIFT(tmp11 - tmp1, CONST_BITS-PASS1_BITS);
    wsptr[7*2] = (int) RIGHT_SHIFT(tmp12 + tmp2, CONST_BITS-PASS1_BITS);
    wsptr[7*4] = (int) RIGHT_SHIFT(tmp12 - tmp2, CONST_BITS-PASS1_BITS);
    wsptr[7*3] = (int) RIGHT_SHIFT(tmp13, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 7; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp13 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp13 <<= CONST_BITS;

    z1 = (INT32) wsptr[2];
    z2 = (INT32) wsptr[4];
    z3 = (INT32) wsptr[6];

    tmp10 = MULTIPLY(z2 - z3, FIX(0.881747734));     
    tmp12 = MULTIPLY(z1 - z2, FIX(0.314692123));     
    tmp11 = tmp10 + tmp12 + tmp13 - MULTIPLY(z2, FIX(1.841218003)); 
    tmp0 = z1 + z3;
    z2 -= tmp0;
    tmp0 = MULTIPLY(tmp0, FIX(1.274162392)) + tmp13; 
    tmp10 += tmp0 - MULTIPLY(z3, FIX(0.077722536));  
    tmp12 += tmp0 - MULTIPLY(z1, FIX(2.470602249));  
    tmp13 += MULTIPLY(z2, FIX(1.414213562));         

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];

    tmp1 = MULTIPLY(z1 + z2, FIX(0.935414347));      
    tmp2 = MULTIPLY(z1 - z2, FIX(0.170262339));      
    tmp0 = tmp1 - tmp2;
    tmp1 += tmp2;
    tmp2 = MULTIPLY(z2 + z3, - FIX(1.378756276));    
    tmp1 += tmp2;
    z2 = MULTIPLY(z1 + z3, FIX(0.613604268));        
    tmp0 += z2;
    tmp2 += z2 + MULTIPLY(z3, FIX(1.870828693));     

    

    outptr[0] = range_limit[(int) RIGHT_SHIFT(tmp10 + tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[6] = range_limit[(int) RIGHT_SHIFT(tmp10 - tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) RIGHT_SHIFT(tmp11 + tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) RIGHT_SHIFT(tmp11 - tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) RIGHT_SHIFT(tmp12 + tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) RIGHT_SHIFT(tmp12 - tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) RIGHT_SHIFT(tmp13,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += 7;		
  }
}










GLOBAL(void)
jpeg_idct_6x6 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp2, tmp10, tmp11, tmp12;
  INT32 z1, z2, z3;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[6*6];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 6; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= CONST_BITS;
    
    tmp0 += ONE << (CONST_BITS-PASS1_BITS-1);
    tmp2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    tmp10 = MULTIPLY(tmp2, FIX(0.707106781));   
    tmp1 = tmp0 + tmp10;
    tmp11 = RIGHT_SHIFT(tmp0 - tmp10 - tmp10, CONST_BITS-PASS1_BITS);
    tmp10 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    tmp0 = MULTIPLY(tmp10, FIX(1.224744871));   
    tmp10 = tmp1 + tmp0;
    tmp12 = tmp1 - tmp0;

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp1 = MULTIPLY(z1 + z3, FIX(0.366025404)); 
    tmp0 = tmp1 + ((z1 + z2) << CONST_BITS);
    tmp2 = tmp1 + ((z3 - z2) << CONST_BITS);
    tmp1 = (z1 - z2 - z3) << PASS1_BITS;

    

    wsptr[6*0] = (int) RIGHT_SHIFT(tmp10 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[6*5] = (int) RIGHT_SHIFT(tmp10 - tmp0, CONST_BITS-PASS1_BITS);
    wsptr[6*1] = (int) (tmp11 + tmp1);
    wsptr[6*4] = (int) (tmp11 - tmp1);
    wsptr[6*2] = (int) RIGHT_SHIFT(tmp12 + tmp2, CONST_BITS-PASS1_BITS);
    wsptr[6*3] = (int) RIGHT_SHIFT(tmp12 - tmp2, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 6; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp0 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp0 <<= CONST_BITS;
    tmp2 = (INT32) wsptr[4];
    tmp10 = MULTIPLY(tmp2, FIX(0.707106781));   
    tmp1 = tmp0 + tmp10;
    tmp11 = tmp0 - tmp10 - tmp10;
    tmp10 = (INT32) wsptr[2];
    tmp0 = MULTIPLY(tmp10, FIX(1.224744871));   
    tmp10 = tmp1 + tmp0;
    tmp12 = tmp1 - tmp0;

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    tmp1 = MULTIPLY(z1 + z3, FIX(0.366025404)); 
    tmp0 = tmp1 + ((z1 + z2) << CONST_BITS);
    tmp2 = tmp1 + ((z3 - z2) << CONST_BITS);
    tmp1 = (z1 - z2 - z3) << CONST_BITS;

    

    outptr[0] = range_limit[(int) RIGHT_SHIFT(tmp10 + tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) RIGHT_SHIFT(tmp10 - tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) RIGHT_SHIFT(tmp11 + tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) RIGHT_SHIFT(tmp11 - tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) RIGHT_SHIFT(tmp12 + tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) RIGHT_SHIFT(tmp12 - tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += 6;		
  }
}










GLOBAL(void)
jpeg_idct_5x5 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp10, tmp11, tmp12;
  INT32 z1, z2, z3;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[5*5];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 5; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp12 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp12 <<= CONST_BITS;
    
    tmp12 += ONE << (CONST_BITS-PASS1_BITS-1);
    tmp0 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    tmp1 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z1 = MULTIPLY(tmp0 + tmp1, FIX(0.790569415)); 
    z2 = MULTIPLY(tmp0 - tmp1, FIX(0.353553391)); 
    z3 = tmp12 + z2;
    tmp10 = z3 + z1;
    tmp11 = z3 - z1;
    tmp12 -= z2 << 2;

    

    z2 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);

    z1 = MULTIPLY(z2 + z3, FIX(0.831253876));     
    tmp0 = z1 + MULTIPLY(z2, FIX(0.513743148));   
    tmp1 = z1 - MULTIPLY(z3, FIX(2.176250899));   

    

    wsptr[5*0] = (int) RIGHT_SHIFT(tmp10 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[5*4] = (int) RIGHT_SHIFT(tmp10 - tmp0, CONST_BITS-PASS1_BITS);
    wsptr[5*1] = (int) RIGHT_SHIFT(tmp11 + tmp1, CONST_BITS-PASS1_BITS);
    wsptr[5*3] = (int) RIGHT_SHIFT(tmp11 - tmp1, CONST_BITS-PASS1_BITS);
    wsptr[5*2] = (int) RIGHT_SHIFT(tmp12, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 5; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp12 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp12 <<= CONST_BITS;
    tmp0 = (INT32) wsptr[2];
    tmp1 = (INT32) wsptr[4];
    z1 = MULTIPLY(tmp0 + tmp1, FIX(0.790569415)); 
    z2 = MULTIPLY(tmp0 - tmp1, FIX(0.353553391)); 
    z3 = tmp12 + z2;
    tmp10 = z3 + z1;
    tmp11 = z3 - z1;
    tmp12 -= z2 << 2;

    

    z2 = (INT32) wsptr[1];
    z3 = (INT32) wsptr[3];

    z1 = MULTIPLY(z2 + z3, FIX(0.831253876));     
    tmp0 = z1 + MULTIPLY(z2, FIX(0.513743148));   
    tmp1 = z1 - MULTIPLY(z3, FIX(2.176250899));   

    

    outptr[0] = range_limit[(int) RIGHT_SHIFT(tmp10 + tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) RIGHT_SHIFT(tmp10 - tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) RIGHT_SHIFT(tmp11 + tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) RIGHT_SHIFT(tmp11 - tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) RIGHT_SHIFT(tmp12,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += 5;		
  }
}










GLOBAL(void)
jpeg_idct_3x3 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp2, tmp10, tmp12;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[3*3];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 3; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= CONST_BITS;
    
    tmp0 += ONE << (CONST_BITS-PASS1_BITS-1);
    tmp2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    tmp12 = MULTIPLY(tmp2, FIX(0.707106781)); 
    tmp10 = tmp0 + tmp12;
    tmp2 = tmp0 - tmp12 - tmp12;

    

    tmp12 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    tmp0 = MULTIPLY(tmp12, FIX(1.224744871)); 

    

    wsptr[3*0] = (int) RIGHT_SHIFT(tmp10 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[3*2] = (int) RIGHT_SHIFT(tmp10 - tmp0, CONST_BITS-PASS1_BITS);
    wsptr[3*1] = (int) RIGHT_SHIFT(tmp2, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 3; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp0 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp0 <<= CONST_BITS;
    tmp2 = (INT32) wsptr[2];
    tmp12 = MULTIPLY(tmp2, FIX(0.707106781)); 
    tmp10 = tmp0 + tmp12;
    tmp2 = tmp0 - tmp12 - tmp12;

    

    tmp12 = (INT32) wsptr[1];
    tmp0 = MULTIPLY(tmp12, FIX(1.224744871)); 

    

    outptr[0] = range_limit[(int) RIGHT_SHIFT(tmp10 + tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) RIGHT_SHIFT(tmp10 - tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) RIGHT_SHIFT(tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += 3;		
  }
}










GLOBAL(void)
jpeg_idct_9x9 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13, tmp14;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*9];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= CONST_BITS;
    
    tmp0 += ONE << (CONST_BITS-PASS1_BITS-1);

    z1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp3 = MULTIPLY(z3, FIX(0.707106781));      
    tmp1 = tmp0 + tmp3;
    tmp2 = tmp0 - tmp3 - tmp3;

    tmp0 = MULTIPLY(z1 - z2, FIX(0.707106781)); 
    tmp11 = tmp2 + tmp0;
    tmp14 = tmp2 - tmp0 - tmp0;

    tmp0 = MULTIPLY(z1 + z2, FIX(1.328926049)); 
    tmp2 = MULTIPLY(z1, FIX(1.083350441));      
    tmp3 = MULTIPLY(z2, FIX(0.245575608));      

    tmp10 = tmp1 + tmp0 - tmp3;
    tmp12 = tmp1 - tmp0 + tmp2;
    tmp13 = tmp1 - tmp2 + tmp3;

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    z2 = MULTIPLY(z2, - FIX(1.224744871));           

    tmp2 = MULTIPLY(z1 + z3, FIX(0.909038955));      
    tmp3 = MULTIPLY(z1 + z4, FIX(0.483689525));      
    tmp0 = tmp2 + tmp3 - z2;
    tmp1 = MULTIPLY(z3 - z4, FIX(1.392728481));      
    tmp2 += z2 - tmp1;
    tmp3 += z2 + tmp1;
    tmp1 = MULTIPLY(z1 - z3 - z4, FIX(1.224744871)); 

    

    wsptr[8*0] = (int) RIGHT_SHIFT(tmp10 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[8*8] = (int) RIGHT_SHIFT(tmp10 - tmp0, CONST_BITS-PASS1_BITS);
    wsptr[8*1] = (int) RIGHT_SHIFT(tmp11 + tmp1, CONST_BITS-PASS1_BITS);
    wsptr[8*7] = (int) RIGHT_SHIFT(tmp11 - tmp1, CONST_BITS-PASS1_BITS);
    wsptr[8*2] = (int) RIGHT_SHIFT(tmp12 + tmp2, CONST_BITS-PASS1_BITS);
    wsptr[8*6] = (int) RIGHT_SHIFT(tmp12 - tmp2, CONST_BITS-PASS1_BITS);
    wsptr[8*3] = (int) RIGHT_SHIFT(tmp13 + tmp3, CONST_BITS-PASS1_BITS);
    wsptr[8*5] = (int) RIGHT_SHIFT(tmp13 - tmp3, CONST_BITS-PASS1_BITS);
    wsptr[8*4] = (int) RIGHT_SHIFT(tmp14, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 9; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp0 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp0 <<= CONST_BITS;

    z1 = (INT32) wsptr[2];
    z2 = (INT32) wsptr[4];
    z3 = (INT32) wsptr[6];

    tmp3 = MULTIPLY(z3, FIX(0.707106781));      
    tmp1 = tmp0 + tmp3;
    tmp2 = tmp0 - tmp3 - tmp3;

    tmp0 = MULTIPLY(z1 - z2, FIX(0.707106781)); 
    tmp11 = tmp2 + tmp0;
    tmp14 = tmp2 - tmp0 - tmp0;

    tmp0 = MULTIPLY(z1 + z2, FIX(1.328926049)); 
    tmp2 = MULTIPLY(z1, FIX(1.083350441));      
    tmp3 = MULTIPLY(z2, FIX(0.245575608));      

    tmp10 = tmp1 + tmp0 - tmp3;
    tmp12 = tmp1 - tmp0 + tmp2;
    tmp13 = tmp1 - tmp2 + tmp3;

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z4 = (INT32) wsptr[7];

    z2 = MULTIPLY(z2, - FIX(1.224744871));           

    tmp2 = MULTIPLY(z1 + z3, FIX(0.909038955));      
    tmp3 = MULTIPLY(z1 + z4, FIX(0.483689525));      
    tmp0 = tmp2 + tmp3 - z2;
    tmp1 = MULTIPLY(z3 - z4, FIX(1.392728481));      
    tmp2 += z2 - tmp1;
    tmp3 += z2 + tmp1;
    tmp1 = MULTIPLY(z1 - z3 - z4, FIX(1.224744871)); 

    

    outptr[0] = range_limit[(int) RIGHT_SHIFT(tmp10 + tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[8] = range_limit[(int) RIGHT_SHIFT(tmp10 - tmp0,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) RIGHT_SHIFT(tmp11 + tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[7] = range_limit[(int) RIGHT_SHIFT(tmp11 - tmp1,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) RIGHT_SHIFT(tmp12 + tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[6] = range_limit[(int) RIGHT_SHIFT(tmp12 - tmp2,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) RIGHT_SHIFT(tmp13 + tmp3,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) RIGHT_SHIFT(tmp13 - tmp3,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) RIGHT_SHIFT(tmp14,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_10x10 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24;
  INT32 z1, z2, z3, z4, z5;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*10];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    z3 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z3 <<= CONST_BITS;
    
    z3 += ONE << (CONST_BITS-PASS1_BITS-1);
    z4 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z1 = MULTIPLY(z4, FIX(1.144122806));         
    z2 = MULTIPLY(z4, FIX(0.437016024));         
    tmp10 = z3 + z1;
    tmp11 = z3 - z2;

    tmp22 = RIGHT_SHIFT(z3 - ((z1 - z2) << 1),   
			CONST_BITS-PASS1_BITS);

    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    z1 = MULTIPLY(z2 + z3, FIX(0.831253876));    
    tmp12 = z1 + MULTIPLY(z2, FIX(0.513743148)); 
    tmp13 = z1 - MULTIPLY(z3, FIX(2.176250899)); 

    tmp20 = tmp10 + tmp12;
    tmp24 = tmp10 - tmp12;
    tmp21 = tmp11 + tmp13;
    tmp23 = tmp11 - tmp13;

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    tmp11 = z2 + z4;
    tmp13 = z2 - z4;

    tmp12 = MULTIPLY(tmp13, FIX(0.309016994));        
    z5 = z3 << CONST_BITS;

    z2 = MULTIPLY(tmp11, FIX(0.951056516));           
    z4 = z5 + tmp12;

    tmp10 = MULTIPLY(z1, FIX(1.396802247)) + z2 + z4; 
    tmp14 = MULTIPLY(z1, FIX(0.221231742)) - z2 + z4; 

    z2 = MULTIPLY(tmp11, FIX(0.587785252));           
    z4 = z5 - tmp12 - (tmp13 << (CONST_BITS - 1));

    tmp12 = (z1 - tmp13 - z3) << PASS1_BITS;

    tmp11 = MULTIPLY(z1, FIX(1.260073511)) - z2 - z4; 
    tmp13 = MULTIPLY(z1, FIX(0.642039522)) - z2 + z4; 

    

    wsptr[8*0] = (int) RIGHT_SHIFT(tmp20 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*9] = (int) RIGHT_SHIFT(tmp20 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*1] = (int) RIGHT_SHIFT(tmp21 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*8] = (int) RIGHT_SHIFT(tmp21 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*2] = (int) (tmp22 + tmp12);
    wsptr[8*7] = (int) (tmp22 - tmp12);
    wsptr[8*3] = (int) RIGHT_SHIFT(tmp23 + tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*6] = (int) RIGHT_SHIFT(tmp23 - tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*4] = (int) RIGHT_SHIFT(tmp24 + tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*5] = (int) RIGHT_SHIFT(tmp24 - tmp14, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 10; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    z3 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    z3 <<= CONST_BITS;
    z4 = (INT32) wsptr[4];
    z1 = MULTIPLY(z4, FIX(1.144122806));         
    z2 = MULTIPLY(z4, FIX(0.437016024));         
    tmp10 = z3 + z1;
    tmp11 = z3 - z2;

    tmp22 = z3 - ((z1 - z2) << 1);               

    z2 = (INT32) wsptr[2];
    z3 = (INT32) wsptr[6];

    z1 = MULTIPLY(z2 + z3, FIX(0.831253876));    
    tmp12 = z1 + MULTIPLY(z2, FIX(0.513743148)); 
    tmp13 = z1 - MULTIPLY(z3, FIX(2.176250899)); 

    tmp20 = tmp10 + tmp12;
    tmp24 = tmp10 - tmp12;
    tmp21 = tmp11 + tmp13;
    tmp23 = tmp11 - tmp13;

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z3 <<= CONST_BITS;
    z4 = (INT32) wsptr[7];

    tmp11 = z2 + z4;
    tmp13 = z2 - z4;

    tmp12 = MULTIPLY(tmp13, FIX(0.309016994));        

    z2 = MULTIPLY(tmp11, FIX(0.951056516));           
    z4 = z3 + tmp12;

    tmp10 = MULTIPLY(z1, FIX(1.396802247)) + z2 + z4; 
    tmp14 = MULTIPLY(z1, FIX(0.221231742)) - z2 + z4; 

    z2 = MULTIPLY(tmp11, FIX(0.587785252));           
    z4 = z3 - tmp12 - (tmp13 << (CONST_BITS - 1));

    tmp12 = ((z1 - tmp13) << CONST_BITS) - z3;

    tmp11 = MULTIPLY(z1, FIX(1.260073511)) - z2 - z4; 
    tmp13 = MULTIPLY(z1, FIX(0.642039522)) - z2 + z4; 

    

    outptr[0] = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp10,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[9] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp10,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp11,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[8] = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp11,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp12,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[7] = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp12,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp13,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[6] = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp13,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp14,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp14,
					      CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_11x11 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24, tmp25;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*11];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp10 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp10 <<= CONST_BITS;
    
    tmp10 += ONE << (CONST_BITS-PASS1_BITS-1);

    z1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp20 = MULTIPLY(z2 - z3, FIX(2.546640132));     
    tmp23 = MULTIPLY(z2 - z1, FIX(0.430815045));     
    z4 = z1 + z3;
    tmp24 = MULTIPLY(z4, - FIX(1.155664402));        
    z4 -= z2;
    tmp25 = tmp10 + MULTIPLY(z4, FIX(1.356927976));  
    tmp21 = tmp20 + tmp23 + tmp25 -
	    MULTIPLY(z2, FIX(1.821790775));          
    tmp20 += tmp25 + MULTIPLY(z3, FIX(2.115825087)); 
    tmp23 += tmp25 - MULTIPLY(z1, FIX(1.513598477)); 
    tmp24 += tmp25;
    tmp22 = tmp24 - MULTIPLY(z3, FIX(0.788749120));  
    tmp24 += MULTIPLY(z2, FIX(1.944413522)) -        
	     MULTIPLY(z1, FIX(1.390975730));         
    tmp25 = tmp10 - MULTIPLY(z4, FIX(1.414213562));  

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    tmp11 = z1 + z2;
    tmp14 = MULTIPLY(tmp11 + z3 + z4, FIX(0.398430003)); 
    tmp11 = MULTIPLY(tmp11, FIX(0.887983902));           
    tmp12 = MULTIPLY(z1 + z3, FIX(0.670361295));         
    tmp13 = tmp14 + MULTIPLY(z1 + z4, FIX(0.366151574)); 
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(z1, FIX(0.923107866));              
    z1    = tmp14 - MULTIPLY(z2 + z3, FIX(1.163011579)); 
    tmp11 += z1 + MULTIPLY(z2, FIX(2.073276588));        
    tmp12 += z1 - MULTIPLY(z3, FIX(1.192193623));        
    z1    = MULTIPLY(z2 + z4, - FIX(1.798248910));       
    tmp11 += z1;
    tmp13 += z1 + MULTIPLY(z4, FIX(2.102458632));        
    tmp14 += MULTIPLY(z2, - FIX(1.467221301)) +          
	     MULTIPLY(z3, FIX(1.001388905)) -            
	     MULTIPLY(z4, FIX(1.684843907));             

    

    wsptr[8*0]  = (int) RIGHT_SHIFT(tmp20 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*10] = (int) RIGHT_SHIFT(tmp20 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*1]  = (int) RIGHT_SHIFT(tmp21 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*9]  = (int) RIGHT_SHIFT(tmp21 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*2]  = (int) RIGHT_SHIFT(tmp22 + tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*8]  = (int) RIGHT_SHIFT(tmp22 - tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*3]  = (int) RIGHT_SHIFT(tmp23 + tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*7]  = (int) RIGHT_SHIFT(tmp23 - tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*4]  = (int) RIGHT_SHIFT(tmp24 + tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*6]  = (int) RIGHT_SHIFT(tmp24 - tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*5]  = (int) RIGHT_SHIFT(tmp25, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 11; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp10 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp10 <<= CONST_BITS;

    z1 = (INT32) wsptr[2];
    z2 = (INT32) wsptr[4];
    z3 = (INT32) wsptr[6];

    tmp20 = MULTIPLY(z2 - z3, FIX(2.546640132));     
    tmp23 = MULTIPLY(z2 - z1, FIX(0.430815045));     
    z4 = z1 + z3;
    tmp24 = MULTIPLY(z4, - FIX(1.155664402));        
    z4 -= z2;
    tmp25 = tmp10 + MULTIPLY(z4, FIX(1.356927976));  
    tmp21 = tmp20 + tmp23 + tmp25 -
	    MULTIPLY(z2, FIX(1.821790775));          
    tmp20 += tmp25 + MULTIPLY(z3, FIX(2.115825087)); 
    tmp23 += tmp25 - MULTIPLY(z1, FIX(1.513598477)); 
    tmp24 += tmp25;
    tmp22 = tmp24 - MULTIPLY(z3, FIX(0.788749120));  
    tmp24 += MULTIPLY(z2, FIX(1.944413522)) -        
	     MULTIPLY(z1, FIX(1.390975730));         
    tmp25 = tmp10 - MULTIPLY(z4, FIX(1.414213562));  

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z4 = (INT32) wsptr[7];

    tmp11 = z1 + z2;
    tmp14 = MULTIPLY(tmp11 + z3 + z4, FIX(0.398430003)); 
    tmp11 = MULTIPLY(tmp11, FIX(0.887983902));           
    tmp12 = MULTIPLY(z1 + z3, FIX(0.670361295));         
    tmp13 = tmp14 + MULTIPLY(z1 + z4, FIX(0.366151574)); 
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(z1, FIX(0.923107866));              
    z1    = tmp14 - MULTIPLY(z2 + z3, FIX(1.163011579)); 
    tmp11 += z1 + MULTIPLY(z2, FIX(2.073276588));        
    tmp12 += z1 - MULTIPLY(z3, FIX(1.192193623));        
    z1    = MULTIPLY(z2 + z4, - FIX(1.798248910));       
    tmp11 += z1;
    tmp13 += z1 + MULTIPLY(z4, FIX(2.102458632));        
    tmp14 += MULTIPLY(z2, - FIX(1.467221301)) +          
	     MULTIPLY(z3, FIX(1.001388905)) -            
	     MULTIPLY(z4, FIX(1.684843907));             

    

    outptr[0]  = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[10] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[1]  = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[9]  = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[2]  = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[8]  = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[3]  = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[7]  = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[4]  = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[6]  = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[5]  = range_limit[(int) RIGHT_SHIFT(tmp25,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_12x12 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24, tmp25;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*12];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    z3 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z3 <<= CONST_BITS;
    
    z3 += ONE << (CONST_BITS-PASS1_BITS-1);

    z4 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z4 = MULTIPLY(z4, FIX(1.224744871)); 

    tmp10 = z3 + z4;
    tmp11 = z3 - z4;

    z1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z4 = MULTIPLY(z1, FIX(1.366025404)); 
    z1 <<= CONST_BITS;
    z2 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);
    z2 <<= CONST_BITS;

    tmp12 = z1 - z2;

    tmp21 = z3 + tmp12;
    tmp24 = z3 - tmp12;

    tmp12 = z4 + z2;

    tmp20 = tmp10 + tmp12;
    tmp25 = tmp10 - tmp12;

    tmp12 = z4 - z1 - z2;

    tmp22 = tmp11 + tmp12;
    tmp23 = tmp11 - tmp12;

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    tmp11 = MULTIPLY(z2, FIX(1.306562965));                  
    tmp14 = MULTIPLY(z2, - FIX_0_541196100);                 

    tmp10 = z1 + z3;
    tmp15 = MULTIPLY(tmp10 + z4, FIX(0.860918669));          
    tmp12 = tmp15 + MULTIPLY(tmp10, FIX(0.261052384));       
    tmp10 = tmp12 + tmp11 + MULTIPLY(z1, FIX(0.280143716));  
    tmp13 = MULTIPLY(z3 + z4, - FIX(1.045510580));           
    tmp12 += tmp13 + tmp14 - MULTIPLY(z3, FIX(1.478575242)); 
    tmp13 += tmp15 - tmp11 + MULTIPLY(z4, FIX(1.586706681)); 
    tmp15 += tmp14 - MULTIPLY(z1, FIX(0.676326758)) -        
	     MULTIPLY(z4, FIX(1.982889723));                 

    z1 -= z4;
    z2 -= z3;
    z3 = MULTIPLY(z1 + z2, FIX_0_541196100);                 
    tmp11 = z3 + MULTIPLY(z1, FIX_0_765366865);              
    tmp14 = z3 - MULTIPLY(z2, FIX_1_847759065);              

    

    wsptr[8*0]  = (int) RIGHT_SHIFT(tmp20 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*11] = (int) RIGHT_SHIFT(tmp20 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*1]  = (int) RIGHT_SHIFT(tmp21 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*10] = (int) RIGHT_SHIFT(tmp21 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*2]  = (int) RIGHT_SHIFT(tmp22 + tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*9]  = (int) RIGHT_SHIFT(tmp22 - tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*3]  = (int) RIGHT_SHIFT(tmp23 + tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*8]  = (int) RIGHT_SHIFT(tmp23 - tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*4]  = (int) RIGHT_SHIFT(tmp24 + tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*7]  = (int) RIGHT_SHIFT(tmp24 - tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*5]  = (int) RIGHT_SHIFT(tmp25 + tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*6]  = (int) RIGHT_SHIFT(tmp25 - tmp15, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 12; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    z3 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    z3 <<= CONST_BITS;

    z4 = (INT32) wsptr[4];
    z4 = MULTIPLY(z4, FIX(1.224744871)); 

    tmp10 = z3 + z4;
    tmp11 = z3 - z4;

    z1 = (INT32) wsptr[2];
    z4 = MULTIPLY(z1, FIX(1.366025404)); 
    z1 <<= CONST_BITS;
    z2 = (INT32) wsptr[6];
    z2 <<= CONST_BITS;

    tmp12 = z1 - z2;

    tmp21 = z3 + tmp12;
    tmp24 = z3 - tmp12;

    tmp12 = z4 + z2;

    tmp20 = tmp10 + tmp12;
    tmp25 = tmp10 - tmp12;

    tmp12 = z4 - z1 - z2;

    tmp22 = tmp11 + tmp12;
    tmp23 = tmp11 - tmp12;

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z4 = (INT32) wsptr[7];

    tmp11 = MULTIPLY(z2, FIX(1.306562965));                  
    tmp14 = MULTIPLY(z2, - FIX_0_541196100);                 

    tmp10 = z1 + z3;
    tmp15 = MULTIPLY(tmp10 + z4, FIX(0.860918669));          
    tmp12 = tmp15 + MULTIPLY(tmp10, FIX(0.261052384));       
    tmp10 = tmp12 + tmp11 + MULTIPLY(z1, FIX(0.280143716));  
    tmp13 = MULTIPLY(z3 + z4, - FIX(1.045510580));           
    tmp12 += tmp13 + tmp14 - MULTIPLY(z3, FIX(1.478575242)); 
    tmp13 += tmp15 - tmp11 + MULTIPLY(z4, FIX(1.586706681)); 
    tmp15 += tmp14 - MULTIPLY(z1, FIX(0.676326758)) -        
	     MULTIPLY(z4, FIX(1.982889723));                 

    z1 -= z4;
    z2 -= z3;
    z3 = MULTIPLY(z1 + z2, FIX_0_541196100);                 
    tmp11 = z3 + MULTIPLY(z1, FIX_0_765366865);              
    tmp14 = z3 - MULTIPLY(z2, FIX_1_847759065);              

    

    outptr[0]  = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[11] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[1]  = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[10] = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[2]  = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[9]  = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[3]  = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[8]  = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[4]  = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[7]  = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[5]  = range_limit[(int) RIGHT_SHIFT(tmp25 + tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[6]  = range_limit[(int) RIGHT_SHIFT(tmp25 - tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_13x13 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*13];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    z1 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z1 <<= CONST_BITS;
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-1);

    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp10 = z3 + z4;
    tmp11 = z3 - z4;

    tmp12 = MULTIPLY(tmp10, FIX(1.155388986));                
    tmp13 = MULTIPLY(tmp11, FIX(0.096834934)) + z1;           

    tmp20 = MULTIPLY(z2, FIX(1.373119086)) + tmp12 + tmp13;   
    tmp22 = MULTIPLY(z2, FIX(0.501487041)) - tmp12 + tmp13;   

    tmp12 = MULTIPLY(tmp10, FIX(0.316450131));                
    tmp13 = MULTIPLY(tmp11, FIX(0.486914739)) + z1;           

    tmp21 = MULTIPLY(z2, FIX(1.058554052)) - tmp12 + tmp13;   
    tmp25 = MULTIPLY(z2, - FIX(1.252223920)) + tmp12 + tmp13; 

    tmp12 = MULTIPLY(tmp10, FIX(0.435816023));                
    tmp13 = MULTIPLY(tmp11, FIX(0.937303064)) - z1;           

    tmp23 = MULTIPLY(z2, - FIX(0.170464608)) - tmp12 - tmp13; 
    tmp24 = MULTIPLY(z2, - FIX(0.803364869)) + tmp12 - tmp13; 

    tmp26 = MULTIPLY(tmp11 - z2, FIX(1.414213562)) + z1;      

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    tmp11 = MULTIPLY(z1 + z2, FIX(1.322312651));     
    tmp12 = MULTIPLY(z1 + z3, FIX(1.163874945));     
    tmp15 = z1 + z4;
    tmp13 = MULTIPLY(tmp15, FIX(0.937797057));       
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(z1, FIX(2.020082300));          
    tmp14 = MULTIPLY(z2 + z3, - FIX(0.338443458));   
    tmp11 += tmp14 + MULTIPLY(z2, FIX(0.837223564)); 
    tmp12 += tmp14 - MULTIPLY(z3, FIX(1.572116027)); 
    tmp14 = MULTIPLY(z2 + z4, - FIX(1.163874945));   
    tmp11 += tmp14;
    tmp13 += tmp14 + MULTIPLY(z4, FIX(2.205608352)); 
    tmp14 = MULTIPLY(z3 + z4, - FIX(0.657217813));   
    tmp12 += tmp14;
    tmp13 += tmp14;
    tmp15 = MULTIPLY(tmp15, FIX(0.338443458));       
    tmp14 = tmp15 + MULTIPLY(z1, FIX(0.318774355)) - 
	    MULTIPLY(z2, FIX(0.466105296));          
    z1    = MULTIPLY(z3 - z2, FIX(0.937797057));     
    tmp14 += z1;
    tmp15 += z1 + MULTIPLY(z3, FIX(0.384515595)) -   
	     MULTIPLY(z4, FIX(1.742345811));         

    

    wsptr[8*0]  = (int) RIGHT_SHIFT(tmp20 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*12] = (int) RIGHT_SHIFT(tmp20 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*1]  = (int) RIGHT_SHIFT(tmp21 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*11] = (int) RIGHT_SHIFT(tmp21 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*2]  = (int) RIGHT_SHIFT(tmp22 + tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*10] = (int) RIGHT_SHIFT(tmp22 - tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*3]  = (int) RIGHT_SHIFT(tmp23 + tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*9]  = (int) RIGHT_SHIFT(tmp23 - tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*4]  = (int) RIGHT_SHIFT(tmp24 + tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*8]  = (int) RIGHT_SHIFT(tmp24 - tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*5]  = (int) RIGHT_SHIFT(tmp25 + tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*7]  = (int) RIGHT_SHIFT(tmp25 - tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*6]  = (int) RIGHT_SHIFT(tmp26, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 13; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    z1 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    z1 <<= CONST_BITS;

    z2 = (INT32) wsptr[2];
    z3 = (INT32) wsptr[4];
    z4 = (INT32) wsptr[6];

    tmp10 = z3 + z4;
    tmp11 = z3 - z4;

    tmp12 = MULTIPLY(tmp10, FIX(1.155388986));                
    tmp13 = MULTIPLY(tmp11, FIX(0.096834934)) + z1;           

    tmp20 = MULTIPLY(z2, FIX(1.373119086)) + tmp12 + tmp13;   
    tmp22 = MULTIPLY(z2, FIX(0.501487041)) - tmp12 + tmp13;   

    tmp12 = MULTIPLY(tmp10, FIX(0.316450131));                
    tmp13 = MULTIPLY(tmp11, FIX(0.486914739)) + z1;           

    tmp21 = MULTIPLY(z2, FIX(1.058554052)) - tmp12 + tmp13;   
    tmp25 = MULTIPLY(z2, - FIX(1.252223920)) + tmp12 + tmp13; 

    tmp12 = MULTIPLY(tmp10, FIX(0.435816023));                
    tmp13 = MULTIPLY(tmp11, FIX(0.937303064)) - z1;           

    tmp23 = MULTIPLY(z2, - FIX(0.170464608)) - tmp12 - tmp13; 
    tmp24 = MULTIPLY(z2, - FIX(0.803364869)) + tmp12 - tmp13; 

    tmp26 = MULTIPLY(tmp11 - z2, FIX(1.414213562)) + z1;      

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z4 = (INT32) wsptr[7];

    tmp11 = MULTIPLY(z1 + z2, FIX(1.322312651));     
    tmp12 = MULTIPLY(z1 + z3, FIX(1.163874945));     
    tmp15 = z1 + z4;
    tmp13 = MULTIPLY(tmp15, FIX(0.937797057));       
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(z1, FIX(2.020082300));          
    tmp14 = MULTIPLY(z2 + z3, - FIX(0.338443458));   
    tmp11 += tmp14 + MULTIPLY(z2, FIX(0.837223564)); 
    tmp12 += tmp14 - MULTIPLY(z3, FIX(1.572116027)); 
    tmp14 = MULTIPLY(z2 + z4, - FIX(1.163874945));   
    tmp11 += tmp14;
    tmp13 += tmp14 + MULTIPLY(z4, FIX(2.205608352)); 
    tmp14 = MULTIPLY(z3 + z4, - FIX(0.657217813));   
    tmp12 += tmp14;
    tmp13 += tmp14;
    tmp15 = MULTIPLY(tmp15, FIX(0.338443458));       
    tmp14 = tmp15 + MULTIPLY(z1, FIX(0.318774355)) - 
	    MULTIPLY(z2, FIX(0.466105296));          
    z1    = MULTIPLY(z3 - z2, FIX(0.937797057));     
    tmp14 += z1;
    tmp15 += z1 + MULTIPLY(z3, FIX(0.384515595)) -   
	     MULTIPLY(z4, FIX(1.742345811));         

    

    outptr[0]  = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[12] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[1]  = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[11] = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[2]  = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[10] = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[3]  = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[9]  = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[4]  = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[8]  = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[5]  = range_limit[(int) RIGHT_SHIFT(tmp25 + tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[7]  = range_limit[(int) RIGHT_SHIFT(tmp25 - tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[6]  = range_limit[(int) RIGHT_SHIFT(tmp26,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_14x14 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*14];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    z1 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z1 <<= CONST_BITS;
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-1);
    z4 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z2 = MULTIPLY(z4, FIX(1.274162392));         
    z3 = MULTIPLY(z4, FIX(0.314692123));         
    z4 = MULTIPLY(z4, FIX(0.881747734));         

    tmp10 = z1 + z2;
    tmp11 = z1 + z3;
    tmp12 = z1 - z4;

    tmp23 = RIGHT_SHIFT(z1 - ((z2 + z3 - z4) << 1), 
			CONST_BITS-PASS1_BITS);

    z1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    z3 = MULTIPLY(z1 + z2, FIX(1.105676686));    

    tmp13 = z3 + MULTIPLY(z1, FIX(0.273079590)); 
    tmp14 = z3 - MULTIPLY(z2, FIX(1.719280954)); 
    tmp15 = MULTIPLY(z1, FIX(0.613604268)) -     
	    MULTIPLY(z2, FIX(1.378756276));      

    tmp20 = tmp10 + tmp13;
    tmp26 = tmp10 - tmp13;
    tmp21 = tmp11 + tmp14;
    tmp25 = tmp11 - tmp14;
    tmp22 = tmp12 + tmp15;
    tmp24 = tmp12 - tmp15;

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    tmp13 = z4 << CONST_BITS;

    tmp14 = z1 + z3;
    tmp11 = MULTIPLY(z1 + z2, FIX(1.334852607));           
    tmp12 = MULTIPLY(tmp14, FIX(1.197448846));             
    tmp10 = tmp11 + tmp12 + tmp13 - MULTIPLY(z1, FIX(1.126980169)); 
    tmp14 = MULTIPLY(tmp14, FIX(0.752406978));             
    tmp16 = tmp14 - MULTIPLY(z1, FIX(1.061150426));        
    z1    -= z2;
    tmp15 = MULTIPLY(z1, FIX(0.467085129)) - tmp13;        
    tmp16 += tmp15;
    z1    += z4;
    z4    = MULTIPLY(z2 + z3, - FIX(0.158341681)) - tmp13; 
    tmp11 += z4 - MULTIPLY(z2, FIX(0.424103948));          
    tmp12 += z4 - MULTIPLY(z3, FIX(2.373959773));          
    z4    = MULTIPLY(z3 - z2, FIX(1.405321284));           
    tmp14 += z4 + tmp13 - MULTIPLY(z3, FIX(1.6906431334)); 
    tmp15 += z4 + MULTIPLY(z2, FIX(0.674957567));          

    tmp13 = (z1 - z3) << PASS1_BITS;

    

    wsptr[8*0]  = (int) RIGHT_SHIFT(tmp20 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*13] = (int) RIGHT_SHIFT(tmp20 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*1]  = (int) RIGHT_SHIFT(tmp21 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*12] = (int) RIGHT_SHIFT(tmp21 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*2]  = (int) RIGHT_SHIFT(tmp22 + tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*11] = (int) RIGHT_SHIFT(tmp22 - tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*3]  = (int) (tmp23 + tmp13);
    wsptr[8*10] = (int) (tmp23 - tmp13);
    wsptr[8*4]  = (int) RIGHT_SHIFT(tmp24 + tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*9]  = (int) RIGHT_SHIFT(tmp24 - tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*5]  = (int) RIGHT_SHIFT(tmp25 + tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*8]  = (int) RIGHT_SHIFT(tmp25 - tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*6]  = (int) RIGHT_SHIFT(tmp26 + tmp16, CONST_BITS-PASS1_BITS);
    wsptr[8*7]  = (int) RIGHT_SHIFT(tmp26 - tmp16, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 14; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    z1 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    z1 <<= CONST_BITS;
    z4 = (INT32) wsptr[4];
    z2 = MULTIPLY(z4, FIX(1.274162392));         
    z3 = MULTIPLY(z4, FIX(0.314692123));         
    z4 = MULTIPLY(z4, FIX(0.881747734));         

    tmp10 = z1 + z2;
    tmp11 = z1 + z3;
    tmp12 = z1 - z4;

    tmp23 = z1 - ((z2 + z3 - z4) << 1);          

    z1 = (INT32) wsptr[2];
    z2 = (INT32) wsptr[6];

    z3 = MULTIPLY(z1 + z2, FIX(1.105676686));    

    tmp13 = z3 + MULTIPLY(z1, FIX(0.273079590)); 
    tmp14 = z3 - MULTIPLY(z2, FIX(1.719280954)); 
    tmp15 = MULTIPLY(z1, FIX(0.613604268)) -     
	    MULTIPLY(z2, FIX(1.378756276));      

    tmp20 = tmp10 + tmp13;
    tmp26 = tmp10 - tmp13;
    tmp21 = tmp11 + tmp14;
    tmp25 = tmp11 - tmp14;
    tmp22 = tmp12 + tmp15;
    tmp24 = tmp12 - tmp15;

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z4 = (INT32) wsptr[7];
    z4 <<= CONST_BITS;

    tmp14 = z1 + z3;
    tmp11 = MULTIPLY(z1 + z2, FIX(1.334852607));           
    tmp12 = MULTIPLY(tmp14, FIX(1.197448846));             
    tmp10 = tmp11 + tmp12 + z4 - MULTIPLY(z1, FIX(1.126980169)); 
    tmp14 = MULTIPLY(tmp14, FIX(0.752406978));             
    tmp16 = tmp14 - MULTIPLY(z1, FIX(1.061150426));        
    z1    -= z2;
    tmp15 = MULTIPLY(z1, FIX(0.467085129)) - z4;           
    tmp16 += tmp15;
    tmp13 = MULTIPLY(z2 + z3, - FIX(0.158341681)) - z4;    
    tmp11 += tmp13 - MULTIPLY(z2, FIX(0.424103948));       
    tmp12 += tmp13 - MULTIPLY(z3, FIX(2.373959773));       
    tmp13 = MULTIPLY(z3 - z2, FIX(1.405321284));           
    tmp14 += tmp13 + z4 - MULTIPLY(z3, FIX(1.6906431334)); 
    tmp15 += tmp13 + MULTIPLY(z2, FIX(0.674957567));       

    tmp13 = ((z1 - z3) << CONST_BITS) + z4;

    

    outptr[0]  = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[13] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[1]  = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[12] = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[2]  = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[11] = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[3]  = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[10] = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[4]  = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[9]  = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[5]  = range_limit[(int) RIGHT_SHIFT(tmp25 + tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[8]  = range_limit[(int) RIGHT_SHIFT(tmp25 - tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[6]  = range_limit[(int) RIGHT_SHIFT(tmp26 + tmp16,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[7]  = range_limit[(int) RIGHT_SHIFT(tmp26 - tmp16,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_15x15 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26, tmp27;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*15];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    z1 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z1 <<= CONST_BITS;
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-1);

    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp10 = MULTIPLY(z4, FIX(0.437016024)); 
    tmp11 = MULTIPLY(z4, FIX(1.144122806)); 

    tmp12 = z1 - tmp10;
    tmp13 = z1 + tmp11;
    z1 -= (tmp11 - tmp10) << 1;             

    z4 = z2 - z3;
    z3 += z2;
    tmp10 = MULTIPLY(z3, FIX(1.337628990)); 
    tmp11 = MULTIPLY(z4, FIX(0.045680613)); 
    z2 = MULTIPLY(z2, FIX(1.439773946));    

    tmp20 = tmp13 + tmp10 + tmp11;
    tmp23 = tmp12 - tmp10 + tmp11 + z2;

    tmp10 = MULTIPLY(z3, FIX(0.547059574)); 
    tmp11 = MULTIPLY(z4, FIX(0.399234004)); 

    tmp25 = tmp13 - tmp10 - tmp11;
    tmp26 = tmp12 + tmp10 - tmp11 - z2;

    tmp10 = MULTIPLY(z3, FIX(0.790569415)); 
    tmp11 = MULTIPLY(z4, FIX(0.353553391)); 

    tmp21 = tmp12 + tmp10 + tmp11;
    tmp24 = tmp13 - tmp10 + tmp11;
    tmp11 += tmp11;
    tmp22 = z1 + tmp11;                     
    tmp27 = z1 - tmp11 - tmp11;             

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z3 = MULTIPLY(z4, FIX(1.224744871));                    
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    tmp13 = z2 - z4;
    tmp15 = MULTIPLY(z1 + tmp13, FIX(0.831253876));         
    tmp11 = tmp15 + MULTIPLY(z1, FIX(0.513743148));         
    tmp14 = tmp15 - MULTIPLY(tmp13, FIX(2.176250899));      

    tmp13 = MULTIPLY(z2, - FIX(0.831253876));               
    tmp15 = MULTIPLY(z2, - FIX(1.344997024));               
    z2 = z1 - z4;
    tmp12 = z3 + MULTIPLY(z2, FIX(1.406466353));            

    tmp10 = tmp12 + MULTIPLY(z4, FIX(2.457431844)) - tmp15; 
    tmp16 = tmp12 - MULTIPLY(z1, FIX(1.112434820)) + tmp13; 
    tmp12 = MULTIPLY(z2, FIX(1.224744871)) - z3;            
    z2 = MULTIPLY(z1 + z4, FIX(0.575212477));               
    tmp13 += z2 + MULTIPLY(z1, FIX(0.475753014)) - z3;      
    tmp15 += z2 - MULTIPLY(z4, FIX(0.869244010)) + z3;      

    

    wsptr[8*0]  = (int) RIGHT_SHIFT(tmp20 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*14] = (int) RIGHT_SHIFT(tmp20 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*1]  = (int) RIGHT_SHIFT(tmp21 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*13] = (int) RIGHT_SHIFT(tmp21 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*2]  = (int) RIGHT_SHIFT(tmp22 + tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*12] = (int) RIGHT_SHIFT(tmp22 - tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*3]  = (int) RIGHT_SHIFT(tmp23 + tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*11] = (int) RIGHT_SHIFT(tmp23 - tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*4]  = (int) RIGHT_SHIFT(tmp24 + tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*10] = (int) RIGHT_SHIFT(tmp24 - tmp14, CONST_BITS-PASS1_BITS);
    wsptr[8*5]  = (int) RIGHT_SHIFT(tmp25 + tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*9]  = (int) RIGHT_SHIFT(tmp25 - tmp15, CONST_BITS-PASS1_BITS);
    wsptr[8*6]  = (int) RIGHT_SHIFT(tmp26 + tmp16, CONST_BITS-PASS1_BITS);
    wsptr[8*8]  = (int) RIGHT_SHIFT(tmp26 - tmp16, CONST_BITS-PASS1_BITS);
    wsptr[8*7]  = (int) RIGHT_SHIFT(tmp27, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 15; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    z1 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    z1 <<= CONST_BITS;

    z2 = (INT32) wsptr[2];
    z3 = (INT32) wsptr[4];
    z4 = (INT32) wsptr[6];

    tmp10 = MULTIPLY(z4, FIX(0.437016024)); 
    tmp11 = MULTIPLY(z4, FIX(1.144122806)); 

    tmp12 = z1 - tmp10;
    tmp13 = z1 + tmp11;
    z1 -= (tmp11 - tmp10) << 1;             

    z4 = z2 - z3;
    z3 += z2;
    tmp10 = MULTIPLY(z3, FIX(1.337628990)); 
    tmp11 = MULTIPLY(z4, FIX(0.045680613)); 
    z2 = MULTIPLY(z2, FIX(1.439773946));    

    tmp20 = tmp13 + tmp10 + tmp11;
    tmp23 = tmp12 - tmp10 + tmp11 + z2;

    tmp10 = MULTIPLY(z3, FIX(0.547059574)); 
    tmp11 = MULTIPLY(z4, FIX(0.399234004)); 

    tmp25 = tmp13 - tmp10 - tmp11;
    tmp26 = tmp12 + tmp10 - tmp11 - z2;

    tmp10 = MULTIPLY(z3, FIX(0.790569415)); 
    tmp11 = MULTIPLY(z4, FIX(0.353553391)); 

    tmp21 = tmp12 + tmp10 + tmp11;
    tmp24 = tmp13 - tmp10 + tmp11;
    tmp11 += tmp11;
    tmp22 = z1 + tmp11;                     
    tmp27 = z1 - tmp11 - tmp11;             

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z4 = (INT32) wsptr[5];
    z3 = MULTIPLY(z4, FIX(1.224744871));                    
    z4 = (INT32) wsptr[7];

    tmp13 = z2 - z4;
    tmp15 = MULTIPLY(z1 + tmp13, FIX(0.831253876));         
    tmp11 = tmp15 + MULTIPLY(z1, FIX(0.513743148));         
    tmp14 = tmp15 - MULTIPLY(tmp13, FIX(2.176250899));      

    tmp13 = MULTIPLY(z2, - FIX(0.831253876));               
    tmp15 = MULTIPLY(z2, - FIX(1.344997024));               
    z2 = z1 - z4;
    tmp12 = z3 + MULTIPLY(z2, FIX(1.406466353));            

    tmp10 = tmp12 + MULTIPLY(z4, FIX(2.457431844)) - tmp15; 
    tmp16 = tmp12 - MULTIPLY(z1, FIX(1.112434820)) + tmp13; 
    tmp12 = MULTIPLY(z2, FIX(1.224744871)) - z3;            
    z2 = MULTIPLY(z1 + z4, FIX(0.575212477));               
    tmp13 += z2 + MULTIPLY(z1, FIX(0.475753014)) - z3;      
    tmp15 += z2 - MULTIPLY(z4, FIX(0.869244010)) + z3;      

    

    outptr[0]  = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[14] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[1]  = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[13] = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[2]  = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[12] = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[3]  = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[11] = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[4]  = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[10] = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp14,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[5]  = range_limit[(int) RIGHT_SHIFT(tmp25 + tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[9]  = range_limit[(int) RIGHT_SHIFT(tmp25 - tmp15,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[6]  = range_limit[(int) RIGHT_SHIFT(tmp26 + tmp16,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[8]  = range_limit[(int) RIGHT_SHIFT(tmp26 - tmp16,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[7]  = range_limit[(int) RIGHT_SHIFT(tmp27,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];

    wsptr += 8;		
  }
}










GLOBAL(void)
jpeg_idct_16x16 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
  INT32 tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26, tmp27;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[8*16];	
  SHIFT_TEMPS

  

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = 0; ctr < 8; ctr++, inptr++, quantptr++, wsptr++) {
    

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= CONST_BITS;
    
    tmp0 += 1 << (CONST_BITS-PASS1_BITS-1);

    z1 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    tmp1 = MULTIPLY(z1, FIX(1.306562965));      
    tmp2 = MULTIPLY(z1, FIX_0_541196100);       

    tmp10 = tmp0 + tmp1;
    tmp11 = tmp0 - tmp1;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp0 - tmp2;

    z1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);
    z3 = z1 - z2;
    z4 = MULTIPLY(z3, FIX(0.275899379));        
    z3 = MULTIPLY(z3, FIX(1.387039845));        

    tmp0 = z3 + MULTIPLY(z2, FIX_2_562915447);  
    tmp1 = z4 + MULTIPLY(z1, FIX_0_899976223);  
    tmp2 = z3 - MULTIPLY(z1, FIX(0.601344887)); 
    tmp3 = z4 - MULTIPLY(z2, FIX(0.509795579)); 

    tmp20 = tmp10 + tmp0;
    tmp27 = tmp10 - tmp0;
    tmp21 = tmp12 + tmp1;
    tmp26 = tmp12 - tmp1;
    tmp22 = tmp13 + tmp2;
    tmp25 = tmp13 - tmp2;
    tmp23 = tmp11 + tmp3;
    tmp24 = tmp11 - tmp3;

    

    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    tmp11 = z1 + z3;

    tmp1  = MULTIPLY(z1 + z2, FIX(1.353318001));   
    tmp2  = MULTIPLY(tmp11,   FIX(1.247225013));   
    tmp3  = MULTIPLY(z1 + z4, FIX(1.093201867));   
    tmp10 = MULTIPLY(z1 - z4, FIX(0.897167586));   
    tmp11 = MULTIPLY(tmp11,   FIX(0.666655658));   
    tmp12 = MULTIPLY(z1 - z2, FIX(0.410524528));   
    tmp0  = tmp1 + tmp2 + tmp3 -
	    MULTIPLY(z1, FIX(2.286341144));        
    tmp13 = tmp10 + tmp11 + tmp12 -
	    MULTIPLY(z1, FIX(1.835730603));        
    z1    = MULTIPLY(z2 + z3, FIX(0.138617169));   
    tmp1  += z1 + MULTIPLY(z2, FIX(0.071888074));  
    tmp2  += z1 - MULTIPLY(z3, FIX(1.125726048));  
    z1    = MULTIPLY(z3 - z2, FIX(1.407403738));   
    tmp11 += z1 - MULTIPLY(z3, FIX(0.766367282));  
    tmp12 += z1 + MULTIPLY(z2, FIX(1.971951411));  
    z2    += z4;
    z1    = MULTIPLY(z2, - FIX(0.666655658));      
    tmp1  += z1;
    tmp3  += z1 + MULTIPLY(z4, FIX(1.065388962));  
    z2    = MULTIPLY(z2, - FIX(1.247225013));      
    tmp10 += z2 + MULTIPLY(z4, FIX(3.141271809));  
    tmp12 += z2;
    z2    = MULTIPLY(z3 + z4, - FIX(1.353318001)); 
    tmp2  += z2;
    tmp3  += z2;
    z2    = MULTIPLY(z4 - z3, FIX(0.410524528));   
    tmp10 += z2;
    tmp11 += z2;

    

    wsptr[8*0]  = (int) RIGHT_SHIFT(tmp20 + tmp0,  CONST_BITS-PASS1_BITS);
    wsptr[8*15] = (int) RIGHT_SHIFT(tmp20 - tmp0,  CONST_BITS-PASS1_BITS);
    wsptr[8*1]  = (int) RIGHT_SHIFT(tmp21 + tmp1,  CONST_BITS-PASS1_BITS);
    wsptr[8*14] = (int) RIGHT_SHIFT(tmp21 - tmp1,  CONST_BITS-PASS1_BITS);
    wsptr[8*2]  = (int) RIGHT_SHIFT(tmp22 + tmp2,  CONST_BITS-PASS1_BITS);
    wsptr[8*13] = (int) RIGHT_SHIFT(tmp22 - tmp2,  CONST_BITS-PASS1_BITS);
    wsptr[8*3]  = (int) RIGHT_SHIFT(tmp23 + tmp3,  CONST_BITS-PASS1_BITS);
    wsptr[8*12] = (int) RIGHT_SHIFT(tmp23 - tmp3,  CONST_BITS-PASS1_BITS);
    wsptr[8*4]  = (int) RIGHT_SHIFT(tmp24 + tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*11] = (int) RIGHT_SHIFT(tmp24 - tmp10, CONST_BITS-PASS1_BITS);
    wsptr[8*5]  = (int) RIGHT_SHIFT(tmp25 + tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*10] = (int) RIGHT_SHIFT(tmp25 - tmp11, CONST_BITS-PASS1_BITS);
    wsptr[8*6]  = (int) RIGHT_SHIFT(tmp26 + tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*9]  = (int) RIGHT_SHIFT(tmp26 - tmp12, CONST_BITS-PASS1_BITS);
    wsptr[8*7]  = (int) RIGHT_SHIFT(tmp27 + tmp13, CONST_BITS-PASS1_BITS);
    wsptr[8*8]  = (int) RIGHT_SHIFT(tmp27 - tmp13, CONST_BITS-PASS1_BITS);
  }

  

  wsptr = workspace;
  for (ctr = 0; ctr < 16; ctr++) {
    outptr = output_buf[ctr] + output_col;

    

    
    tmp0 = (INT32) wsptr[0] + (ONE << (PASS1_BITS+2));
    tmp0 <<= CONST_BITS;

    z1 = (INT32) wsptr[4];
    tmp1 = MULTIPLY(z1, FIX(1.306562965));      
    tmp2 = MULTIPLY(z1, FIX_0_541196100);       

    tmp10 = tmp0 + tmp1;
    tmp11 = tmp0 - tmp1;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp0 - tmp2;

    z1 = (INT32) wsptr[2];
    z2 = (INT32) wsptr[6];
    z3 = z1 - z2;
    z4 = MULTIPLY(z3, FIX(0.275899379));        
    z3 = MULTIPLY(z3, FIX(1.387039845));        

    tmp0 = z3 + MULTIPLY(z2, FIX_2_562915447);  
    tmp1 = z4 + MULTIPLY(z1, FIX_0_899976223);  
    tmp2 = z3 - MULTIPLY(z1, FIX(0.601344887)); 
    tmp3 = z4 - MULTIPLY(z2, FIX(0.509795579)); 

    tmp20 = tmp10 + tmp0;
    tmp27 = tmp10 - tmp0;
    tmp21 = tmp12 + tmp1;
    tmp26 = tmp12 - tmp1;
    tmp22 = tmp13 + tmp2;
    tmp25 = tmp13 - tmp2;
    tmp23 = tmp11 + tmp3;
    tmp24 = tmp11 - tmp3;

    

    z1 = (INT32) wsptr[1];
    z2 = (INT32) wsptr[3];
    z3 = (INT32) wsptr[5];
    z4 = (INT32) wsptr[7];

    tmp11 = z1 + z3;

    tmp1  = MULTIPLY(z1 + z2, FIX(1.353318001));   
    tmp2  = MULTIPLY(tmp11,   FIX(1.247225013));   
    tmp3  = MULTIPLY(z1 + z4, FIX(1.093201867));   
    tmp10 = MULTIPLY(z1 - z4, FIX(0.897167586));   
    tmp11 = MULTIPLY(tmp11,   FIX(0.666655658));   
    tmp12 = MULTIPLY(z1 - z2, FIX(0.410524528));   
    tmp0  = tmp1 + tmp2 + tmp3 -
	    MULTIPLY(z1, FIX(2.286341144));        
    tmp13 = tmp10 + tmp11 + tmp12 -
	    MULTIPLY(z1, FIX(1.835730603));        
    z1    = MULTIPLY(z2 + z3, FIX(0.138617169));   
    tmp1  += z1 + MULTIPLY(z2, FIX(0.071888074));  
    tmp2  += z1 - MULTIPLY(z3, FIX(1.125726048));  
    z1    = MULTIPLY(z3 - z2, FIX(1.407403738));   
    tmp11 += z1 - MULTIPLY(z3, FIX(0.766367282));  
    tmp12 += z1 + MULTIPLY(z2, FIX(1.971951411));  
    z2    += z4;
    z1    = MULTIPLY(z2, - FIX(0.666655658));      
    tmp1  += z1;
    tmp3  += z1 + MULTIPLY(z4, FIX(1.065388962));  
    z2    = MULTIPLY(z2, - FIX(1.247225013));      
    tmp10 += z2 + MULTIPLY(z4, FIX(3.141271809));  
    tmp12 += z2;
    z2    = MULTIPLY(z3 + z4, - FIX(1.353318001)); 
    tmp2  += z2;
    tmp3  += z2;
    z2    = MULTIPLY(z4 - z3, FIX(0.410524528));   
    tmp10 += z2;
    tmp11 += z2;

    

    outptr[0]  = range_limit[(int) RIGHT_SHIFT(tmp20 + tmp0,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[15] = range_limit[(int) RIGHT_SHIFT(tmp20 - tmp0,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[1]  = range_limit[(int) RIGHT_SHIFT(tmp21 + tmp1,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[14] = range_limit[(int) RIGHT_SHIFT(tmp21 - tmp1,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[2]  = range_limit[(int) RIGHT_SHIFT(tmp22 + tmp2,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[13] = range_limit[(int) RIGHT_SHIFT(tmp22 - tmp2,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[3]  = range_limit[(int) RIGHT_SHIFT(tmp23 + tmp3,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[12] = range_limit[(int) RIGHT_SHIFT(tmp23 - tmp3,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[4]  = range_limit[(int) RIGHT_SHIFT(tmp24 + tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[11] = range_limit[(int) RIGHT_SHIFT(tmp24 - tmp10,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[5]  = range_limit[(int) RIGHT_SHIFT(tmp25 + tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[10] = range_limit[(int) RIGHT_SHIFT(tmp25 - tmp11,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[6]  = range_limit[(int) RIGHT_SHIFT(tmp26 + tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[9]  = range_limit[(int) RIGHT_SHIFT(tmp26 - tmp12,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[7]  = range_limit[(int) RIGHT_SHIFT(tmp27 + tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];
    outptr[8]  = range_limit[(int) RIGHT_SHIFT(tmp27 - tmp13,
					       CONST_BITS+PASS1_BITS+3)
			     & RANGE_MASK];

    wsptr += 8;		
  }
}

#endif 
#endif 
