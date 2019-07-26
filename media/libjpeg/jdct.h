




























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
typedef UINT32 UDCTELEM;
typedef unsigned long long UDCTELEM2;
#endif



















typedef MULTIPLIER ISLOW_MULT_TYPE; 
#if BITS_IN_JSAMPLE == 8
typedef MULTIPLIER IFAST_MULT_TYPE; 
#define IFAST_SCALE_BITS  2	/* fractional bits in scale factors */
#else
typedef INT32 IFAST_MULT_TYPE;	
#define IFAST_SCALE_BITS  13	/* fractional bits in scale factors */
#endif
typedef FAST_FLOAT FLOAT_MULT_TYPE; 











#define IDCT_range_limit(cinfo)  ((cinfo)->sample_range_limit + CENTERJSAMPLE)

#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */




#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_fdct_islow		jFDislow
#define jpeg_fdct_ifast		jFDifast
#define jpeg_fdct_float		jFDfloat
#define jpeg_idct_islow		jRDislow
#define jpeg_idct_ifast		jRDifast
#define jpeg_idct_float		jRDfloat
#define jpeg_idct_7x7		jRD7x7
#define jpeg_idct_6x6		jRD6x6
#define jpeg_idct_5x5		jRD5x5
#define jpeg_idct_4x4		jRD4x4
#define jpeg_idct_3x3		jRD3x3
#define jpeg_idct_2x2		jRD2x2
#define jpeg_idct_1x1		jRD1x1
#define jpeg_idct_9x9		jRD9x9
#define jpeg_idct_10x10		jRD10x10
#define jpeg_idct_11x11		jRD11x11
#define jpeg_idct_12x12		jRD12x12
#define jpeg_idct_13x13		jRD13x13
#define jpeg_idct_14x14		jRD14x14
#define jpeg_idct_15x15		jRD15x15
#define jpeg_idct_16x16		jRD16x16
#endif 



EXTERN(void) jpeg_fdct_islow JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_ifast JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_float JPP((FAST_FLOAT * data));

EXTERN(void) jpeg_idct_islow
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_ifast
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_float
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_7x7
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_6x6
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_5x5
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x4
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_3x3
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_1x1
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_9x9
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_10x10
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_11x11
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_12x12
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_13x13
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_14x14
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_15x15
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_16x16
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));












#define ONE	((INT32) 1)
#define CONST_SCALE (ONE << CONST_BITS)






#define FIX(x)	((INT32) ((x) * CONST_SCALE + 0.5))






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
