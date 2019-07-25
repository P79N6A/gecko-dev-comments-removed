




















#ifndef PNGCONF_H
#define PNGCONF_H

#include "mozpngconf.h"

#ifndef PNG_NO_LIMITS_H
#  include <limits.h>
#endif






#ifdef PNG_CONFIGURE_LIBPNG
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif







#ifdef PNG_USER_CONFIG
#  include "pngusr.h"
#  ifndef PNG_USER_PRIVATEBUILD
#    define PNG_USER_PRIVATEBUILD
#  endif
#endif




















#ifdef __STDC__
#  ifdef SPECIALBUILD
#    pragma message("PNG_LIBPNG_SPECIALBUILD (and deprecated SPECIALBUILD)\
     are now LIBPNG reserved macros. Use PNG_USER_PRIVATEBUILD instead.")
#  endif

#  ifdef PRIVATEBUILD
#    pragma message("PRIVATEBUILD is deprecated.\
     Use PNG_USER_PRIVATEBUILD instead.")
#    define PNG_USER_PRIVATEBUILD PRIVATEBUILD
#  endif
#endif 




#ifndef PNG_UNUSED






#  define PNG_UNUSED(param) (void)param;
#endif


#ifndef PNG_VERSION_INFO_ONLY














#ifndef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 8192
#endif



#ifndef PNG_NO_READ_SUPPORTED
#  define PNG_READ_SUPPORTED
#endif



#ifndef PNG_NO_WRITE_SUPPORTED
#  define PNG_WRITE_SUPPORTED
#endif


#ifdef PNG_ALLOW_BENIGN_ERRORS
#  define png_benign_error png_warning
#  define png_chunk_benign_error png_chunk_warning
#else
#  ifndef PNG_BENIGN_ERRORS_SUPPORTED
#    define png_benign_error png_error
#    define png_chunk_benign_error png_chunk_error
#  endif
#endif


#if !defined(PNG_NO_WARNINGS) && !defined(PNG_WARNINGS_SUPPORTED)
#  define PNG_WARNINGS_SUPPORTED
#endif


#if !defined(PNG_NO_ERROR_TEXT) && !defined(PNG_ERROR_TEXT_SUPPORTED)
#  define PNG_ERROR_TEXT_SUPPORTED
#endif


#if !defined(PNG_NO_CHECK_cHRM) && !defined(PNG_CHECK_cHRM_SUPPORTED)
#  define PNG_CHECK_cHRM_SUPPORTED
#endif


#if !defined(PNG_NO_ALIGNED_MEMORY) && !defined(PNG_ALIGNED_MEMORY_SUPPORTED)
#  define PNG_ALIGNED_MEMORY_SUPPORTED
#endif



#ifndef PNG_NO_MNG_FEATURES
#  ifndef PNG_MNG_FEATURES_SUPPORTED
#    define PNG_MNG_FEATURES_SUPPORTED
#  endif
#endif


#ifndef PNG_NO_FLOATING_POINT_SUPPORTED
#  ifndef PNG_FLOATING_POINT_SUPPORTED
#    define PNG_FLOATING_POINT_SUPPORTED
#  endif
#endif




#define PNG_CALLOC_SUPPORTED









#if defined(MAXSEG_64K) && !defined(PNG_MAX_MALLOC_64K)
#  define PNG_MAX_MALLOC_64K
#endif


































#ifdef __CYGWIN__
#  ifdef ALL_STATIC
#    ifdef PNG_BUILD_DLL
#      undef PNG_BUILD_DLL
#    endif
#    ifdef PNG_USE_DLL
#      undef PNG_USE_DLL
#    endif
#    ifdef PNG_DLL
#      undef PNG_DLL
#    endif
#    ifndef PNG_STATIC
#      define PNG_STATIC
#    endif
#  else
#    ifdef PNG_BUILD_DLL
#      ifdef PNG_STATIC
#        undef PNG_STATIC
#      endif
#      ifdef PNG_USE_DLL
#        undef PNG_USE_DLL
#      endif
#      ifndef PNG_DLL
#        define PNG_DLL
#      endif
#    else
#      ifdef PNG_STATIC
#        ifdef PNG_USE_DLL
#          undef PNG_USE_DLL
#        endif
#        ifdef PNG_DLL
#          undef PNG_DLL
#        endif
#      else
#        ifndef PNG_USE_DLL
#          define PNG_USE_DLL
#        endif
#        ifndef PNG_DLL
#          define PNG_DLL
#        endif
#      endif
#    endif
#  endif
#endif














#ifdef _WIN32_WCE
#  define PNG_NO_CONSOLE_IO
#  define PNG_NO_STDIO
#  define PNG_NO_TIME_RFC1123
#  ifdef PNG_DEBUG
#    undef PNG_DEBUG
#  endif
#endif

#if !defined(PNG_NO_STDIO) && !defined(PNG_STDIO_SUPPORTED)
#  define PNG_STDIO_SUPPORTED
#endif

#ifdef PNG_BUILD_DLL
#  if !defined(PNG_CONSOLE_IO_SUPPORTED) && !defined(PNG_NO_CONSOLE_IO)
#    define PNG_NO_CONSOLE_IO
#  endif
#endif

#  ifdef PNG_NO_STDIO
#    ifndef PNG_NO_CONSOLE_IO
#      define PNG_NO_CONSOLE_IO
#    endif
#    ifdef PNG_DEBUG
#      if (PNG_DEBUG > 0)
#        include <stdio.h>
#      endif
#    endif
#  else
#    include <stdio.h>
#  endif

#if !(defined PNG_NO_CONSOLE_IO) && !defined(PNG_CONSOLE_IO_SUPPORTED)
#  define PNG_CONSOLE_IO_SUPPORTED
#endif








#ifndef PNGARG

#ifdef OF 
#  define PNGARG(arglist) OF(arglist)
#else

#ifdef _NO_PROTO
#  define PNGARG(arglist) ()
#else
#  define PNGARG(arglist) arglist
#endif 

#endif 

#endif 





#ifndef MACOS
#  if (defined(__MWERKS__) && defined(macintosh)) || defined(applec) || \
      defined(THINK_C) || defined(__SC__) || defined(TARGET_OS_MAC)
#    define MACOS
#  endif
#endif


#if !defined(MACOS) && !defined(RISCOS)
#  include <sys/types.h>
#endif


#if !defined(PNG_NO_SETJMP) && \
    !defined(PNG_SETJMP_NOT_SUPPORTED) && !defined(PNG_NO_SETJMP_SUPPORTED)
#  define PNG_SETJMP_SUPPORTED
#endif

#ifdef PNG_SETJMP_SUPPORTED










#  ifndef PNG_SKIP_SETJMP_CHECK
#    ifdef __linux__
#      ifdef _BSD_SOURCE
#        define PNG_SAVE_BSD_SOURCE
#        undef _BSD_SOURCE
#      endif
#      ifdef _SETJMP_H
       


           __pngconf.h__ in libpng already includes setjmp.h;
           __dont__ include it again.;
#      endif
#    endif 
#  endif 

   
#  include <setjmp.h>

#  ifdef __linux__
#    ifdef PNG_SAVE_BSD_SOURCE
#      ifdef _BSD_SOURCE
#        undef _BSD_SOURCE
#      endif
#      define _BSD_SOURCE
#      undef PNG_SAVE_BSD_SOURCE
#    endif
#  endif 
#endif 

#ifdef BSD
#  include <strings.h>
#else
#  include <string.h>
#endif











#ifdef PNG_DITHER_RED_BITS
#  define PNG_QUANTIZE_RED_BITS PNG_DITHER_RED_BITS
#endif
#ifdef PNG_DITHER_GREEN_BITS
#  define PNG_QUANTIZE_GREEN_BITS PNG_DITHER_GREEN_BITS
#endif
#ifdef PNG_DITHER_BLUE_BITS
#  define PNG_QUANTIZE_BLUE_BITS PNG_DITHER_BLUE_BITS
#endif

#ifndef PNG_QUANTIZE_RED_BITS
#  define PNG_QUANTIZE_RED_BITS 5
#endif
#ifndef PNG_QUANTIZE_GREEN_BITS
#  define PNG_QUANTIZE_GREEN_BITS 5
#endif
#ifndef PNG_QUANTIZE_BLUE_BITS
#  define PNG_QUANTIZE_BLUE_BITS 5
#endif








#ifndef PNG_MAX_GAMMA_8
#  define PNG_MAX_GAMMA_8 11
#endif




#ifndef PNG_GAMMA_THRESHOLD
#  define PNG_GAMMA_THRESHOLD 0.05
#endif






#ifndef PNG_CONST
#  ifndef PNG_NO_CONST
#    define PNG_CONST const
#  else
#    define PNG_CONST
#  endif
#endif































#if !defined(PNG_FLOATING_POINT_SUPPORTED) || \
    !defined(PNG_NO_FIXED_POINT_SUPPORTED)
#  define PNG_FIXED_POINT_SUPPORTED
#endif

#ifdef PNG_READ_SUPPORTED


#if !defined(PNG_READ_TRANSFORMS_NOT_SUPPORTED) && \
      !defined(PNG_NO_READ_TRANSFORMS)
#  define PNG_READ_TRANSFORMS_SUPPORTED
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
#  ifndef PNG_NO_READ_EXPAND
#    define PNG_READ_EXPAND_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SHIFT
#    define PNG_READ_SHIFT_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_PACK
#    define PNG_READ_PACK_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_BGR
#    define PNG_READ_BGR_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SWAP
#    define PNG_READ_SWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_PACKSWAP
#    define PNG_READ_PACKSWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_INVERT
#    define PNG_READ_INVERT_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_QUANTIZE
     
#    ifndef PNG_NO_READ_DITHER  
#      define PNG_READ_QUANTIZE_SUPPORTED
#    endif
#  endif
#  ifndef PNG_NO_READ_BACKGROUND
#    define PNG_READ_BACKGROUND_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_16_TO_8
#    define PNG_READ_16_TO_8_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_FILLER
#    define PNG_READ_FILLER_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_GAMMA
#    define PNG_READ_GAMMA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_GRAY_TO_RGB
#    define PNG_READ_GRAY_TO_RGB_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SWAP_ALPHA
#    define PNG_READ_SWAP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_INVERT_ALPHA
#    define PNG_READ_INVERT_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_STRIP_ALPHA
#    define PNG_READ_STRIP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_USER_TRANSFORM
#    define PNG_READ_USER_TRANSFORM_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_RGB_TO_GRAY
#    define PNG_READ_RGB_TO_GRAY_SUPPORTED
#  endif
#endif 


#if !defined(PNG_NO_PROGRESSIVE_READ) && \
 !defined(PNG_PROGRESSIVE_READ_NOT_SUPPORTED)  
#  define PNG_PROGRESSIVE_READ_SUPPORTED
#endif                               
            

#define PNG_READ_INTERLACING_SUPPORTED


#if !defined(PNG_NO_SEQUENTIAL_READ) && \
    !defined(PNG_SEQUENTIAL_READ_SUPPORTED) && \
    !defined(PNG_NO_SEQUENTIAL_READ_SUPPORTED)
#  define PNG_SEQUENTIAL_READ_SUPPORTED
#endif

#ifndef PNG_NO_READ_COMPOSITE_NODIV
#  ifndef PNG_NO_READ_COMPOSITED_NODIV  
#    define PNG_READ_COMPOSITE_NODIV_SUPPORTED
#  endif
#endif

#if !defined(PNG_NO_GET_INT_32) || defined(PNG_READ_oFFS_SUPPORTED) || \
    defined(PNG_READ_pCAL_SUPPORTED)
#  ifndef PNG_GET_INT_32_SUPPORTED
#    define PNG_GET_INT_32_SUPPORTED
#  endif
#endif

#endif 

#ifdef PNG_WRITE_SUPPORTED


#if !defined(PNG_WRITE_TRANSFORMS_NOT_SUPPORTED) && \
    !defined(PNG_NO_WRITE_TRANSFORMS)
#  define PNG_WRITE_TRANSFORMS_SUPPORTED
#endif

#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
#  ifndef PNG_NO_WRITE_SHIFT
#    define PNG_WRITE_SHIFT_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_PACK
#    define PNG_WRITE_PACK_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_BGR
#    define PNG_WRITE_BGR_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_SWAP
#    define PNG_WRITE_SWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_PACKSWAP
#    define PNG_WRITE_PACKSWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_INVERT
#    define PNG_WRITE_INVERT_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_FILLER
#    define PNG_WRITE_FILLER_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_SWAP_ALPHA
#    define PNG_WRITE_SWAP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_INVERT_ALPHA
#    define PNG_WRITE_INVERT_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_USER_TRANSFORM
#    define PNG_WRITE_USER_TRANSFORM_SUPPORTED
#  endif
#endif 

#if !defined(PNG_NO_WRITE_INTERLACING_SUPPORTED) && \
    !defined(PNG_WRITE_INTERLACING_SUPPORTED)
    


#  define PNG_WRITE_INTERLACING_SUPPORTED
#endif

#if !defined(PNG_NO_WRITE_WEIGHTED_FILTER) && \
    !defined(PNG_WRITE_WEIGHTED_FILTER) && \
     defined(PNG_FLOATING_POINT_SUPPORTED)
#  define PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
#endif

#ifndef PNG_NO_WRITE_FLUSH
#  define PNG_WRITE_FLUSH_SUPPORTED
#endif

#if !defined(PNG_NO_SAVE_INT_32) || defined(PNG_WRITE_oFFS_SUPPORTED) || \
    defined(PNG_WRITE_pCAL_SUPPORTED)
#  ifndef PNG_SAVE_INT_32_SUPPORTED
#    define PNG_SAVE_INT_32_SUPPORTED
#  endif
#endif

#endif 

#define PNG_NO_ERROR_NUMBERS

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
#  ifndef PNG_NO_USER_TRANSFORM_PTR
#    define PNG_USER_TRANSFORM_PTR_SUPPORTED
#  endif
#endif

#if defined(PNG_STDIO_SUPPORTED) && !defined(PNG_TIME_RFC1123_SUPPORTED)
#  define PNG_TIME_RFC1123_SUPPORTED
#endif

















#if !defined(PNG_NO_EASY_ACCESS) && !defined(PNG_EASY_ACCESS_SUPPORTED)
#  define PNG_EASY_ACCESS_SUPPORTED
#endif


#if !defined(PNG_NO_USER_MEM) && !defined(PNG_USER_MEM_SUPPORTED)
#  define PNG_USER_MEM_SUPPORTED
#endif


#ifndef PNG_NO_SET_USER_LIMITS
#  ifndef PNG_SET_USER_LIMITS_SUPPORTED
#    define PNG_SET_USER_LIMITS_SUPPORTED
#  endif
  
#  ifndef PNG_SET_CHUNK_CACHE_LIMIT_SUPPORTED
#    define PNG_SET_CHUNK_CACHE_LIMIT_SUPPORTED
#  endif
  
#  ifndef PNG_SET_CHUNK_MALLOC_LIMIT_SUPPORTED
#    define PNG_SET_CHUNK_MALLOC_LIMIT_SUPPORTED
#  endif
#endif


#ifndef PNG_USER_LIMITS_SUPPORTED
#  ifndef PNG_NO_USER_LIMITS
#    define PNG_USER_LIMITS_SUPPORTED
#  endif
#endif




#ifndef PNG_USER_WIDTH_MAX
#  define PNG_USER_WIDTH_MAX 1000000L
#endif
#ifndef PNG_USER_HEIGHT_MAX
#  define PNG_USER_HEIGHT_MAX 1000000L
#endif




#ifndef PNG_USER_CHUNK_CACHE_MAX
#  define PNG_USER_CHUNK_CACHE_MAX 0
#endif


#ifndef PNG_USER_CHUNK_MALLOC_MAX
#  define PNG_USER_CHUNK_MALLOC_MAX 0
#endif


#if !defined(PNG_NO_IO_STATE) && !defined(PNG_IO_STATE_SUPPORTED)
#  define PNG_IO_STATE_SUPPORTED
#endif

#ifndef PNG_LITERAL_SHARP
#  define PNG_LITERAL_SHARP 0x23
#endif
#ifndef PNG_LITERAL_LEFT_SQUARE_BRACKET
#  define PNG_LITERAL_LEFT_SQUARE_BRACKET 0x5b
#endif
#ifndef PNG_LITERAL_RIGHT_SQUARE_BRACKET
#  define PNG_LITERAL_RIGHT_SQUARE_BRACKET 0x5d
#endif
#ifndef PNG_STRING_NEWLINE
#define PNG_STRING_NEWLINE "\n"
#endif




















#if !defined(PNG_NO_USE_READ_MACROS) && !defined(PNG_USE_READ_MACROS)
#  define PNG_USE_READ_MACROS
#endif



#if !defined(PNG_NO_POINTER_INDEXING) && \
    !defined(PNG_POINTER_INDEXING_SUPPORTED)
#  define PNG_POINTER_INDEXING_SUPPORTED
#endif
















#if defined(PNG_READ_SUPPORTED) && \
    !defined(PNG_READ_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_READ_ANCILLARY_CHUNKS)
#  define PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#endif


#if defined(PNG_WRITE_SUPPORTED) && \
    !defined(PNG_WRITE_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_WRITE_ANCILLARY_CHUNKS)
#  define PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED
#endif

#ifdef PNG_READ_ANCILLARY_CHUNKS_SUPPORTED

#ifdef PNG_NO_READ_TEXT
#  define PNG_NO_READ_iTXt
#  define PNG_NO_READ_tEXt
#  define PNG_NO_READ_zTXt
#endif

#ifndef PNG_NO_READ_bKGD
#  define PNG_READ_bKGD_SUPPORTED
#  define PNG_bKGD_SUPPORTED
#endif
#ifndef PNG_NO_READ_cHRM
#  define PNG_READ_cHRM_SUPPORTED
#  define PNG_cHRM_SUPPORTED
#endif
#ifndef PNG_NO_READ_gAMA
#  define PNG_READ_gAMA_SUPPORTED
#  define PNG_gAMA_SUPPORTED
#endif
#ifndef PNG_NO_READ_hIST
#  define PNG_READ_hIST_SUPPORTED
#  define PNG_hIST_SUPPORTED
#endif
#ifndef PNG_NO_READ_iCCP
#  define PNG_READ_iCCP_SUPPORTED
#  define PNG_iCCP_SUPPORTED
#endif
#ifndef PNG_NO_READ_iTXt
#  ifndef PNG_READ_iTXt_SUPPORTED
#    define PNG_READ_iTXt_SUPPORTED
#  endif
#  ifndef PNG_iTXt_SUPPORTED
#    define PNG_iTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_READ_oFFs
#  define PNG_READ_oFFs_SUPPORTED
#  define PNG_oFFs_SUPPORTED
#endif
#ifndef PNG_NO_READ_pCAL
#  define PNG_READ_pCAL_SUPPORTED
#  define PNG_pCAL_SUPPORTED
#endif
#ifndef PNG_NO_READ_sCAL
#  define PNG_READ_sCAL_SUPPORTED
#  define PNG_sCAL_SUPPORTED
#endif
#ifndef PNG_NO_READ_pHYs
#  define PNG_READ_pHYs_SUPPORTED
#  define PNG_pHYs_SUPPORTED
#endif
#ifndef PNG_NO_READ_sBIT
#  define PNG_READ_sBIT_SUPPORTED
#  define PNG_sBIT_SUPPORTED
#endif
#ifndef PNG_NO_READ_sPLT
#  define PNG_READ_sPLT_SUPPORTED
#  define PNG_sPLT_SUPPORTED
#endif
#ifndef PNG_NO_READ_sRGB
#  define PNG_READ_sRGB_SUPPORTED
#  define PNG_sRGB_SUPPORTED
#endif
#ifndef PNG_NO_READ_tEXt
#  define PNG_READ_tEXt_SUPPORTED
#  define PNG_tEXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_tIME
#  define PNG_READ_tIME_SUPPORTED
#  define PNG_tIME_SUPPORTED
#endif
#ifndef PNG_NO_READ_tRNS
#  define PNG_READ_tRNS_SUPPORTED
#  define PNG_tRNS_SUPPORTED
#endif
#ifndef PNG_NO_READ_APNG
#  define PNG_READ_APNG_SUPPORTED
#  define PNG_APNG_SUPPORTED
#endif
#ifndef PNG_NO_READ_zTXt
#  define PNG_READ_zTXt_SUPPORTED
#  define PNG_zTXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_OPT_PLTE
#  define PNG_READ_OPT_PLTE_SUPPORTED
#endif                      
#if defined(PNG_READ_iTXt_SUPPORTED) || defined(PNG_READ_tEXt_SUPPORTED) || \
    defined(PNG_READ_zTXt_SUPPORTED)
#  define PNG_READ_TEXT_SUPPORTED
#  define PNG_TEXT_SUPPORTED
#endif

#endif 

#ifndef PNG_NO_READ_UNKNOWN_CHUNKS
#  ifndef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_READ_USER_CHUNKS_SUPPORTED
#    define PNG_READ_USER_CHUNKS_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_READ_USER_CHUNKS
#  ifndef PNG_READ_USER_CHUNKS_SUPPORTED
#    define PNG_READ_USER_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_USER_CHUNKS_SUPPORTED
#    define PNG_USER_CHUNKS_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_HANDLE_AS_UNKNOWN
#  ifndef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#    define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#  endif
#endif

#ifdef PNG_WRITE_SUPPORTED
#ifdef PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED

#ifdef PNG_NO_WRITE_TEXT
#  define PNG_NO_WRITE_iTXt
#  define PNG_NO_WRITE_tEXt
#  define PNG_NO_WRITE_zTXt
#endif
#ifndef PNG_NO_WRITE_bKGD
#  define PNG_WRITE_bKGD_SUPPORTED
#  ifndef PNG_bKGD_SUPPORTED
#    define PNG_bKGD_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_cHRM
#  define PNG_WRITE_cHRM_SUPPORTED
#  ifndef PNG_cHRM_SUPPORTED
#    define PNG_cHRM_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_gAMA
#  define PNG_WRITE_gAMA_SUPPORTED
#  ifndef PNG_gAMA_SUPPORTED
#    define PNG_gAMA_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_hIST
#  define PNG_WRITE_hIST_SUPPORTED
#  ifndef PNG_hIST_SUPPORTED
#    define PNG_hIST_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_iCCP
#  define PNG_WRITE_iCCP_SUPPORTED
#  ifndef PNG_iCCP_SUPPORTED
#    define PNG_iCCP_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_iTXt
#  ifndef PNG_WRITE_iTXt_SUPPORTED
#    define PNG_WRITE_iTXt_SUPPORTED
#  endif
#  ifndef PNG_iTXt_SUPPORTED
#    define PNG_iTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_oFFs
#  define PNG_WRITE_oFFs_SUPPORTED
#  ifndef PNG_oFFs_SUPPORTED
#    define PNG_oFFs_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_pCAL
#  define PNG_WRITE_pCAL_SUPPORTED
#  ifndef PNG_pCAL_SUPPORTED
#    define PNG_pCAL_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sCAL
#  define PNG_WRITE_sCAL_SUPPORTED
#  ifndef PNG_sCAL_SUPPORTED
#    define PNG_sCAL_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_pHYs
#  define PNG_WRITE_pHYs_SUPPORTED
#  ifndef PNG_pHYs_SUPPORTED
#    define PNG_pHYs_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sBIT
#  define PNG_WRITE_sBIT_SUPPORTED
#  ifndef PNG_sBIT_SUPPORTED
#    define PNG_sBIT_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sPLT
#  define PNG_WRITE_sPLT_SUPPORTED
#  ifndef PNG_sPLT_SUPPORTED
#    define PNG_sPLT_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sRGB
#  define PNG_WRITE_sRGB_SUPPORTED
#  ifndef PNG_sRGB_SUPPORTED
#    define PNG_sRGB_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tEXt
#  define PNG_WRITE_tEXt_SUPPORTED
#  ifndef PNG_tEXt_SUPPORTED
#    define PNG_tEXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tIME
#  define PNG_WRITE_tIME_SUPPORTED
#  ifndef PNG_tIME_SUPPORTED
#    define PNG_tIME_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tRNS
#  define PNG_WRITE_tRNS_SUPPORTED
#  ifndef PNG_tRNS_SUPPORTED
#    define PNG_tRNS_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_zTXt
#  define PNG_WRITE_zTXt_SUPPORTED
#  ifndef PNG_zTXt_SUPPORTED
#    define PNG_zTXt_SUPPORTED
#  endif
#endif
#if defined(PNG_WRITE_iTXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED) || \
    defined(PNG_WRITE_zTXt_SUPPORTED)
#  define PNG_WRITE_TEXT_SUPPORTED
#  ifndef PNG_TEXT_SUPPORTED
#    define PNG_TEXT_SUPPORTED
#  endif
#endif

#ifdef PNG_WRITE_tIME_SUPPORTED
#  ifndef PNG_NO_CONVERT_tIME
#    ifndef _WIN32_WCE

#      ifndef PNG_CONVERT_tIME_SUPPORTED
#        define PNG_CONVERT_tIME_SUPPORTED
#      endif
#   endif
#  endif
#endif

#ifndef PNG_NO_WRITE_APNG
#  ifndef PNG_WRITE_APNG_SUPPORTED
#    define PNG_WRITE_APNG_SUPPORTED
#  endif
#  ifndef PNG_APNG_SUPPORTED
#    define PNG_APNG_SUPPORTED
#  endif
#endif

#endif 

#ifndef PNG_NO_WRITE_FILTER
#  ifndef PNG_WRITE_FILTER_SUPPORTED
#    define PNG_WRITE_FILTER_SUPPORTED
#  endif
#endif

#ifndef PNG_NO_WRITE_UNKNOWN_CHUNKS
#  define PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
#  ifndef PNG_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_HANDLE_AS_UNKNOWN
#  ifndef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#    define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#  endif
#endif
#endif 





#ifndef PNG_NO_INFO_IMAGE
#  define PNG_INFO_IMAGE_SUPPORTED
#endif


#ifdef PNG_CONVERT_tIME_SUPPORTED
     
#    include <time.h>
#endif









#if defined(INT_MAX) && (INT_MAX > 0x7ffffffeL)
typedef unsigned int png_uint_32;
typedef int png_int_32;
#else
typedef unsigned long png_uint_32;
typedef long png_int_32;
#endif
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

#ifdef PNG_NO_SIZE_T
   typedef unsigned int png_size_t;
#else
   typedef size_t png_size_t;
#endif
#define png_sizeof(x) (sizeof (x))













#ifdef __BORLANDC__
#  if defined(__LARGE__) || defined(__HUGE__) || defined(__COMPACT__)
#    define LDATA 1
#  else
#    define LDATA 0
#  endif
   
#  if !defined(__WIN32__) && !defined(__FLAT__) && !defined(__CYGWIN__)
#    define PNG_MAX_MALLOC_64K
#    if (LDATA != 1)
#      ifndef FAR
#        define FAR __far
#      endif
#      define USE_FAR_KEYWORD
#    endif   
     




#  endif  
#endif   








#ifdef FAR
#  ifdef M_I86MM
#    define USE_FAR_KEYWORD
#    define FARDATA FAR
#    include <dos.h>
#  endif
#endif


#ifndef FAR
#  define FAR
#endif


#ifndef FARDATA
#  define FARDATA
#endif



typedef png_int_32 png_fixed_point;


typedef void            FAR * png_voidp;
typedef png_byte        FAR * png_bytep;
typedef png_uint_32     FAR * png_uint_32p;
typedef png_int_32      FAR * png_int_32p;
typedef png_uint_16     FAR * png_uint_16p;
typedef png_int_16      FAR * png_int_16p;
typedef PNG_CONST char  FAR * png_const_charp;
typedef char            FAR * png_charp;
typedef png_fixed_point FAR * png_fixed_point_p;

#ifndef PNG_NO_STDIO
typedef FILE                * png_FILE_p;
#endif

#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * png_doublep;
#endif


typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;
typedef png_fixed_point FAR * FAR * png_fixed_point_pp;
#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * FAR * png_doublepp;
#endif


typedef char            FAR * FAR * FAR * png_charppp;


















#if !defined(PNG_DLL) && (defined(PNG_BUILD_DLL) || defined(PNG_USE_DLL))
#  define PNG_DLL
#endif








#ifdef __CYGWIN__
#  undef PNGAPI
#  define PNGAPI __cdecl
#  undef PNG_IMPEXP
#  define PNG_IMPEXP
#endif

#ifdef __WATCOMC__
#  ifndef PNGAPI
#    define PNGAPI
#  endif
#endif

#if defined(__MINGW32__) && !defined(PNG_MODULEDEF)
#  ifndef PNG_NO_MODULEDEF
#    define PNG_NO_MODULEDEF
#  endif
#endif

#if !defined(PNG_IMPEXP) && defined(PNG_BUILD_DLL) && !defined(PNG_NO_MODULEDEF)
#  define PNG_IMPEXP
#endif

#if defined(PNG_DLL) || defined(_DLL) || defined(__DLL__ ) || \
    (( defined(_Windows) || defined(_WINDOWS) || \
       defined(WIN32) || defined(_WIN32) || defined(__WIN32__) ))

#  ifndef PNGAPI
#     if defined(__GNUC__) || (defined (_MSC_VER) && (_MSC_VER >= 800))
#        define PNGAPI __cdecl
#     else
#        define PNGAPI _cdecl
#     endif
#  endif

#  if !defined(PNG_IMPEXP) && (!defined(PNG_DLL) || \
       0 )
#     define PNG_IMPEXP
#  endif

#  ifndef PNG_IMPEXP

#    define PNG_EXPORT_TYPE1(type,symbol)  PNG_IMPEXP type PNGAPI symbol
#    define PNG_EXPORT_TYPE2(type,symbol)  type PNG_IMPEXP PNGAPI symbol

     
#    if defined(_MSC_VER) || defined(__BORLANDC__)
#      if (_MSC_VER >= 800) || (__BORLANDC__ >= 0x500)
#         define PNG_EXPORT PNG_EXPORT_TYPE1
#      else
#         define PNG_EXPORT PNG_EXPORT_TYPE2
#         ifdef PNG_BUILD_DLL
#            define PNG_IMPEXP __export
#         else
#            define PNG_IMPEXP
#         endif                              

#      endif
#    endif

#    ifndef PNG_IMPEXP
#      ifdef PNG_BUILD_DLL
#        define PNG_IMPEXP __declspec(dllexport)
#      else
#        define PNG_IMPEXP __declspec(dllimport)
#      endif
#    endif
#  endif  
#else 
#   if (defined(__IBMC__) || defined(__IBMCPP__)) && defined(__OS2__)
#     ifndef PNGAPI
#       define PNGAPI _System
#     endif
#   else
#     if 0 
#     endif
#   endif
#endif

#ifndef PNGAPI
#  define PNGAPI
#endif
#ifndef PNG_IMPEXP
#  define PNG_IMPEXP
#endif

#ifdef PNG_BUILDSYMS
#  ifndef PNG_EXPORT
#    define PNG_EXPORT(type,symbol) PNG_FUNCTION_EXPORT symbol END
#  endif
#endif

#ifndef PNG_EXPORT
#  define PNG_EXPORT(type,symbol) PNG_IMPEXP type PNGAPI symbol
#endif

#define PNG_USE_LOCAL_ARRAYS








#ifndef PNG_NO_PEDANTIC_WARNINGS
#  ifndef PNG_PEDANTIC_WARNINGS_SUPPORTED
#    define PNG_PEDANTIC_WARNINGS_SUPPORTED
#  endif
#endif

#ifdef PNG_PEDANTIC_WARNINGS_SUPPORTED





#  ifdef __GNUC__
#    ifndef PNG_USE_RESULT
#      define PNG_USE_RESULT __attribute__((__warn_unused_result__))
#    endif
#    ifndef PNG_NORETURN
#      define PNG_NORETURN   __attribute__((__noreturn__))
#    endif
#    ifndef PNG_ALLOCATED
#      define PNG_ALLOCATED  __attribute__((__malloc__))
#    endif
#    ifndef PNG_DEPRECATED
#      define PNG_DEPRECATED __attribute__((__deprecated__))
#    endif

    



#    ifndef PNG_DEPSTRUCT
#      define PNG_DEPSTRUCT  __attribute__((__deprecated__))
#    endif
#    ifndef PNG_PRIVATE
#      if 0 
#        define PNG_PRIVATE \
          __attribute__((warning("This function is not exported by libpng.")))
#      else
#        define PNG_PRIVATE \
          __attribute__((__deprecated__))
#      endif
#    endif 
#  endif 
#endif 

#ifndef PNG_DEPRECATED
#  define PNG_DEPRECATED
#endif
#ifndef PNG_USE_RESULT
#  define PNG_USE_RESULT
#endif
#ifndef PNG_NORETURN
#  define PNG_NORETURN
#endif
#ifndef PNG_ALLOCATED
#  define PNG_ALLOCATED
#endif
#ifndef PNG_DEPSTRUCT
#  define PNG_DEPSTRUCT
#endif
#ifndef PNG_PRIVATE
#  define PNG_PRIVATE
#endif






#ifndef PNG_ABORT
#  if (defined(_Windows) || defined(_WINDOWS) || defined(_WINDOWS_))
#     define PNG_ABORT() ExitProcess(0)
#  else
#     define PNG_ABORT() abort()
#  endif
#endif

#ifdef USE_FAR_KEYWORD

#  define CHECK   1
#  define NOCHECK 0
#  define CVT_PTR(ptr) (png_far_to_near(png_ptr,ptr,CHECK))
#  define CVT_PTR_NOCHECK(ptr) (png_far_to_near(png_ptr,ptr,NOCHECK))
#  define png_strcpy  _fstrcpy
#  define png_strncpy _fstrncpy   /* Added to v 1.2.6 */
#  define png_strlen  _fstrlen
#  define png_memcmp  _fmemcmp    /* SJT: added */
#  define png_memcpy  _fmemcpy
#  define png_memset  _fmemset
#  define png_sprintf sprintf
#else
#  if (defined(_Windows) || defined(_WINDOWS) || defined(_WINDOWS_))
#    
#    define CVT_PTR(ptr)         (ptr)
#    define CVT_PTR_NOCHECK(ptr) (ptr)
#    define png_strcpy  lstrcpyA
#    define png_strncpy lstrcpynA
#    define png_strlen  lstrlenA
#    define png_memcmp  memcmp
#    define png_memcpy  CopyMemory
#    define png_memset  memset
#    define png_sprintf wsprintfA
#  else
#    define CVT_PTR(ptr)         (ptr)
#    define CVT_PTR_NOCHECK(ptr) (ptr)
#    define png_strcpy  strcpy
#    define png_strncpy strncpy     /* Added to v 1.2.6 */
#    define png_strlen  strlen
#    define png_memcmp  memcmp      /* SJT: added */
#    define png_memcpy  memcpy
#    define png_memset  memset
#    define png_sprintf sprintf
#  endif
#endif

#ifndef PNG_NO_SNPRINTF
#  ifdef _MSC_VER
#    define png_snprintf _snprintf   /* Added to v 1.2.19 */
#    define png_snprintf2 _snprintf
#    define png_snprintf6 _snprintf
#  else
#    define png_snprintf snprintf   /* Added to v 1.2.19 */
#    define png_snprintf2 snprintf
#    define png_snprintf6 snprintf
#  endif
#else
   





#  define png_snprintf(s1,n,fmt,x1) png_sprintf(s1,fmt,x1)
#  define png_snprintf2(s1,n,fmt,x1,x2) png_sprintf(s1,fmt,x1,x2)
#  define png_snprintf6(s1,n,fmt,x1,x2,x3,x4,x5,x6) \
      png_sprintf(s1,fmt,x1,x2,x3,x4,x5,x6)
#endif










#if defined(__TURBOC__) && !defined(__FLAT__)
   typedef unsigned long png_alloc_size_t;
#else
#  if defined(_MSC_VER) && defined(MAXSEG_64K)
     typedef unsigned long    png_alloc_size_t;
#  else
     



#    if (defined(_Windows) || defined(_WINDOWS) || defined(_WINDOWS_)) && \
        (!defined(INT_MAX) || INT_MAX <= 0x7ffffffeL)
       typedef DWORD         png_alloc_size_t;
#    else
       typedef png_size_t    png_alloc_size_t;
#    endif
#  endif
#endif





#if (PNG_ZBUF_SIZE > 65536L) && defined(PNG_MAX_MALLOC_64K)
#  undef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 65536L
#endif



#endif 

#endif 
