




























#define BITS_IN_JSAMPLE  8	/* use 8 or 12 */











#define MAX_COMPONENTS  10	/* maximum number of image components */
















#if BITS_IN_JSAMPLE == 8




#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#else 

typedef char JSAMPLE;
#ifdef CHAR_IS_UNSIGNED
#define GETJSAMPLE(value)  ((int) (value))
#else
#define GETJSAMPLE(value)  ((int) (value) & 0xFF)
#endif 

#endif 

#define MAXJSAMPLE	255
#define CENTERJSAMPLE	128

#endif 


#if BITS_IN_JSAMPLE == 12




typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	4095
#define CENTERJSAMPLE	2048

#endif 








typedef short JCOEF;



#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__)
#define HAVE_MMX_INTEL_MNEMONICS 



#endif







#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JOCTET;
#define GETJOCTET(value)  (value)

#else 

typedef char JOCTET;
#ifdef CHAR_IS_UNSIGNED
#define GETJOCTET(value)  (value)
#else
#define GETJOCTET(value)  ((value) & 0xFF)
#endif 

#endif 











#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char UINT8;
#else 
#ifdef CHAR_IS_UNSIGNED
typedef char UINT8;
#else 
typedef short UINT8;
#endif 
#endif 



#ifdef HAVE_UNSIGNED_SHORT
typedef unsigned short UINT16;
#else 
typedef unsigned int UINT16;
#endif 



#ifndef XMD_H			
typedef short INT16;
#endif



#ifndef XMD_H			
#ifndef _BASETSD_H_		
#ifndef _BASETSD_H
typedef long INT32;
#endif
#endif
#endif








typedef unsigned int JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L  /* a tad under 64K to prevent overflows */













#include "prtypes.h"


#define METHODDEF(type)		static type PR_CALLBACK

#define LOCAL(type)		static type

PR_BEGIN_EXTERN_C
#define GLOBAL(type) type
#define EXTERN(type) extern type
PR_END_EXTERN_C








#ifdef HAVE_PROTOTYPES
#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist
#else
#define JMETHOD(type,methodname,arglist)  type (*methodname) ()
#endif








#ifndef FAR
#ifdef NEED_FAR_POINTERS
#define FAR  far
#else
#define FAR
#endif
#endif


















#if defined(MUST_UNDEF_HAVE_BOOLEAN_AFTER_INCLUDES) && defined(HAVE_BOOLEAN)
#undef HAVE_BOOLEAN
#endif
#ifndef HAVE_BOOLEAN
typedef unsigned char boolean;
#endif
#ifndef FALSE			
#define FALSE	0		/* values of boolean */
#endif
#ifndef TRUE
#define TRUE	1
#endif









#ifdef JPEG_INTERNALS
#define JPEG_INTERNAL_OPTIONS
#endif

#ifdef JPEG_INTERNAL_OPTIONS























#define DCT_ISLOW_SUPPORTED
#undef  DCT_IFAST_SUPPORTED	/* faster, less accurate integer method */
#undef  DCT_FLOAT_SUPPORTED	/* floating-point: accurate, fast on fast HW */



#undef  C_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#define C_MULTISCAN_FILES_SUPPORTED
#define C_PROGRESSIVE_SUPPORTED
#define ENTROPY_OPT_SUPPORTED








#undef  INPUT_SMOOTHING_SUPPORTED   /* Input image smoothing option? */



#undef  D_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#define D_MULTISCAN_FILES_SUPPORTED
#define D_PROGRESSIVE_SUPPORTED
#define SAVE_MARKERS_SUPPORTED
#define BLOCK_SMOOTHING_SUPPORTED
#undef  IDCT_SCALING_SUPPORTED	    /* Output rescaling via IDCT? */
#undef  UPSAMPLE_SCALING_SUPPORTED  /* Output rescaling at upsample stage? */
#define UPSAMPLE_MERGING_SUPPORTED
#undef  QUANT_1PASS_SUPPORTED	    /* 1-pass color quantization? */
#undef  QUANT_2PASS_SUPPORTED	    /* 2-pass color quantization? */



















#define RGB_RED		0	/* Offset of Red in an RGB scanline element */
#define RGB_GREEN	1	/* Offset of Green */
#define RGB_BLUE	2	/* Offset of Blue */
#define RGB_PIXELSIZE	3	/* JSAMPLEs per RGB scanline element */











#ifndef INLINE
#ifdef __GNUC__			
#define INLINE __inline__
#endif
#if defined( __IBMC__ ) || defined (__IBMCPP__)
#define INLINE _Inline
#endif
#ifndef INLINE
#ifdef __cplusplus
#define INLINE inline		/* a C++ compiler should have it too */
#else
#define INLINE
#endif
#endif
#endif







#ifndef MULTIPLIER
#define MULTIPLIER  int16		/* type for fastest integer multiply */
#endif










#ifndef FAST_FLOAT
#ifdef HAVE_PROTOTYPES
#define FAST_FLOAT  float
#else
#define FAST_FLOAT  double
#endif
#endif

#endif 
