












#include "prtypes.h"
#include "mozilla/StandardInteger.h"










#define BITS_IN_JSAMPLE  8	/* use 8 or 12 */











#define MAX_COMPONENTS  10	/* maximum number of image components */
















#if BITS_IN_JSAMPLE == 8




#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#else 

typedef char JSAMPLE;
#ifdef __CHAR_UNSIGNED__
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








#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JOCTET;
#define GETJOCTET(value)  (value)

#else 

typedef char JOCTET;
#ifdef __CHAR_UNSIGNED__
#define GETJOCTET(value)  (value)
#else
#define GETJOCTET(value)  ((value) & 0xFF)
#endif 

#endif 











typedef uint8_t UINT8;



typedef uint16_t UINT16;



typedef int16_t INT16;



typedef int32_t INT32;








typedef unsigned int JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L  /* a tad under 64K to prevent overflows */










#define METHODDEF(type)		static type

#define LOCAL(type)		static type

#define GLOBAL(type)		type

#define EXTERN(type)		extern type








#ifdef HAVE_PROTOTYPES
#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist
#else
#define JMETHOD(type,methodname,arglist)  type (*methodname) ()
#endif








#ifdef NEED_FAR_POINTERS
#define FAR  far
#else
#define FAR
#endif









#ifndef HAVE_BOOLEAN
typedef int boolean;
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
#define DCT_IFAST_SUPPORTED
#define DCT_FLOAT_SUPPORTED



#define C_MULTISCAN_FILES_SUPPORTED
#define C_PROGRESSIVE_SUPPORTED
#define ENTROPY_OPT_SUPPORTED








#define INPUT_SMOOTHING_SUPPORTED



#define D_MULTISCAN_FILES_SUPPORTED
#define D_PROGRESSIVE_SUPPORTED
#define SAVE_MARKERS_SUPPORTED
#define BLOCK_SMOOTHING_SUPPORTED
#define IDCT_SCALING_SUPPORTED
#undef  UPSAMPLE_SCALING_SUPPORTED  /* Output rescaling at upsample stage? */
#define UPSAMPLE_MERGING_SUPPORTED
#define QUANT_1PASS_SUPPORTED
#define QUANT_2PASS_SUPPORTED



















#define RGB_RED		0	/* Offset of Red in an RGB scanline element */
#define RGB_GREEN	1	/* Offset of Green */
#define RGB_BLUE	2	/* Offset of Blue */
#define RGB_PIXELSIZE	3	/* JSAMPLEs per RGB scanline element */

#define JPEG_NUMCS 16

#define EXT_RGB_RED        0
#define EXT_RGB_GREEN      1
#define EXT_RGB_BLUE       2
#define EXT_RGB_PIXELSIZE  3

#define EXT_RGBX_RED       0
#define EXT_RGBX_GREEN     1
#define EXT_RGBX_BLUE      2
#define EXT_RGBX_PIXELSIZE 4

#define EXT_BGR_RED        2
#define EXT_BGR_GREEN      1
#define EXT_BGR_BLUE       0
#define EXT_BGR_PIXELSIZE  3

#define EXT_BGRX_RED       2
#define EXT_BGRX_GREEN     1
#define EXT_BGRX_BLUE      0
#define EXT_BGRX_PIXELSIZE 4

#define EXT_XBGR_RED       3
#define EXT_XBGR_GREEN     2
#define EXT_XBGR_BLUE      1
#define EXT_XBGR_PIXELSIZE 4

#define EXT_XRGB_RED       1
#define EXT_XRGB_GREEN     2
#define EXT_XRGB_BLUE      3
#define EXT_XRGB_PIXELSIZE 4

static const int rgb_red[JPEG_NUMCS] = {
  -1, -1, RGB_RED, -1, -1, -1, EXT_RGB_RED, EXT_RGBX_RED,
  EXT_BGR_RED, EXT_BGRX_RED, EXT_XBGR_RED, EXT_XRGB_RED,
  EXT_RGBX_RED, EXT_BGRX_RED, EXT_XBGR_RED, EXT_XRGB_RED
};

static const int rgb_green[JPEG_NUMCS] = {
  -1, -1, RGB_GREEN, -1, -1, -1, EXT_RGB_GREEN, EXT_RGBX_GREEN,
  EXT_BGR_GREEN, EXT_BGRX_GREEN, EXT_XBGR_GREEN, EXT_XRGB_GREEN,
  EXT_RGBX_GREEN, EXT_BGRX_GREEN, EXT_XBGR_GREEN, EXT_XRGB_GREEN
};

static const int rgb_blue[JPEG_NUMCS] = {
  -1, -1, RGB_BLUE, -1, -1, -1, EXT_RGB_BLUE, EXT_RGBX_BLUE,
  EXT_BGR_BLUE, EXT_BGRX_BLUE, EXT_XBGR_BLUE, EXT_XRGB_BLUE,
  EXT_RGBX_BLUE, EXT_BGRX_BLUE, EXT_XBGR_BLUE, EXT_XRGB_BLUE
};

static const int rgb_pixelsize[JPEG_NUMCS] = {
  -1, -1, RGB_PIXELSIZE, -1, -1, -1, EXT_RGB_PIXELSIZE, EXT_RGBX_PIXELSIZE,
  EXT_BGR_PIXELSIZE, EXT_BGRX_PIXELSIZE, EXT_XBGR_PIXELSIZE, EXT_XRGB_PIXELSIZE,
  EXT_RGBX_PIXELSIZE, EXT_BGRX_PIXELSIZE, EXT_XBGR_PIXELSIZE, EXT_XRGB_PIXELSIZE
};








#ifndef MULTIPLIER
#ifndef WITH_SIMD
#define MULTIPLIER  int		/* type for fastest integer multiply */
#else
#define MULTIPLIER short  /* prefer 16-bit with SIMD for parellelism */
#endif
#endif










#ifndef FAST_FLOAT
#ifdef HAVE_PROTOTYPES
#define FAST_FLOAT  float
#else
#define FAST_FLOAT  double
#endif
#endif

#endif 
