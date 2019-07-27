






























#if BITS_IN_JSAMPLE == 8
#ifndef WITH_SIMD
typedef int DCTELEM;            
typedef unsigned int UDCTELEM;
typedef unsigned long long UDCTELEM2;
#else
typedef short DCTELEM;  
typedef unsigned short UDCTELEM;
typedef unsigned int UDCTELEM2;
#endif
#else
typedef INT32 DCTELEM;          
typedef unsigned long long UDCTELEM2;
#endif



















typedef MULTIPLIER ISLOW_MULT_TYPE; 
#if BITS_IN_JSAMPLE == 8
typedef MULTIPLIER IFAST_MULT_TYPE; 
#define IFAST_SCALE_BITS  2     /* fractional bits in scale factors */
#else
typedef INT32 IFAST_MULT_TYPE;  
#define IFAST_SCALE_BITS  13    /* fractional bits in scale factors */
#endif
typedef FAST_FLOAT FLOAT_MULT_TYPE; 











#define IDCT_range_limit(cinfo)  ((cinfo)->sample_range_limit + CENTERJSAMPLE)

#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */




EXTERN(void) jpeg_fdct_islow (DCTELEM * data);
EXTERN(void) jpeg_fdct_ifast (DCTELEM * data);
EXTERN(void) jpeg_fdct_float (FAST_FLOAT * data);

EXTERN(void) jpeg_idct_islow
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_ifast
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_float
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_7x7
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_6x6
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_5x5
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_4x4
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_3x3
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_2x2
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_1x1
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_9x9
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_10x10
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_11x11
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_12x12
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_13x13
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_14x14
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_15x15
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
EXTERN(void) jpeg_idct_16x16
        (j_decompress_ptr cinfo, jpeg_component_info * compptr,
         JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);












#define ONE     ((INT32) 1)
#define CONST_SCALE (ONE << CONST_BITS)






#define FIX(x)  ((INT32) ((x) * CONST_SCALE + 0.5))






#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)










#ifdef SHORTxSHORT_32           
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT16) (const)))
#endif
#ifdef SHORTxLCONST_32          
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT32) (const)))
#endif

#ifndef MULTIPLY16C16           
#define MULTIPLY16C16(var,const)  ((var) * (const))
#endif



#ifdef SHORTxSHORT_32           
#define MULTIPLY16V16(var1,var2)  (((INT16) (var1)) * ((INT16) (var2)))
#endif

#ifndef MULTIPLY16V16           
#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))
#endif
