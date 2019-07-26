























#ifndef PNGPRIV_H
#define PNGPRIV_H













#define _POSIX_SOURCE 1 /* Just the POSIX 1003.1 and C89 APIs */




#include <stdlib.h>


#include <stddef.h>

#define PNGLIB_BUILD

#ifdef PNG_USER_CONFIG
#  include "pngusr.h"
   
#  ifndef PNG_USER_PRIVATEBUILD
#    define PNG_USER_PRIVATEBUILD "Custom libpng build"
#  endif
#  ifndef PNG_USER_DLLFNAME_POSTFIX
#    define PNG_USER_DLLFNAME_POSTFIX "Cb"
#  endif
#endif









#ifndef PNG_BUILD_DLL
#  ifdef DLL_EXPORT
      



#     define PNG_BUILD_DLL
#  else
#     ifdef _WINDLL
         





#        define PNG_BUILD_DLL
#     else
#        ifdef __DLL__
            


#           define PNG_BUILD_DLL
#        else
            
#        endif
#     endif
#  endif
#endif 









#ifndef PNG_IMPEXP
#  ifdef PNG_BUILD_DLL
#     define PNG_IMPEXP PNG_DLL_EXPORT
#  else
      


#     define PNG_IMPEXP
#  endif
#endif


#ifndef PNG_DEPRECATED
#  define PNG_DEPRECATED
#endif
#ifndef PNG_PRIVATE
#  define PNG_PRIVATE
#endif

#include "png.h"
#include "pnginfo.h"
#include "pngstruct.h"


#ifndef PNG_DLL_EXPORT
#  define PNG_DLL_EXPORT
#endif











#ifdef PNG_SAFE_LIMITS_SUPPORTED
   
#  ifndef PNG_USER_WIDTH_MAX
#     define PNG_USER_WIDTH_MAX 1000000
#  endif
#  ifndef PNG_USER_HEIGHT_MAX
#     define PNG_USER_HEIGHT_MAX 1000000
#  endif
#  ifndef PNG_USER_CHUNK_CACHE_MAX
#     define PNG_USER_CHUNK_CACHE_MAX 128
#  endif
#  ifndef PNG_USER_CHUNK_MALLOC_MAX
#     define PNG_USER_CHUNK_MALLOC_MAX 8000000
#  endif
#else
   
#  ifndef PNG_USER_WIDTH_MAX
#     define PNG_USER_WIDTH_MAX 0x7fffffff
#  endif
#  ifndef PNG_USER_HEIGHT_MAX
#     define PNG_USER_HEIGHT_MAX 0x7fffffff
#  endif
#  ifndef PNG_USER_CHUNK_CACHE_MAX
#     define PNG_USER_CHUNK_CACHE_MAX 0
#  endif
#  ifndef PNG_USER_CHUNK_MALLOC_MAX
#     define PNG_USER_CHUNK_MALLOC_MAX 0
#  endif
#endif




typedef PNG_CONST png_uint_16p FAR * png_const_uint_16pp;








#ifdef PNG_CONFIGURE_LIBPNG
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

















#if defined(MAXSEG_64K) && !defined(PNG_MAX_MALLOC_64K)
#  define PNG_MAX_MALLOC_64K
#endif

#ifndef PNG_UNUSED






#  define PNG_UNUSED(param) (void)param;
#endif




#if (PNG_ZBUF_SIZE > 65536L) && defined(PNG_MAX_MALLOC_64K)
#  undef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 65536L
#endif




#ifndef PNG_STATIC
#   define PNG_STATIC static
#endif





#ifdef PNG_CONFIGURE_LIBPNG
#  define PNG_RESTRICT restrict
#else
   


#  if defined __GNUC__ || defined _MSC_VER || defined __WATCOMC__
#     define PNG_RESTRICT restrict
#  else
#     define PNG_RESTRICT
#  endif
#endif





#ifdef PNG_WARNINGS_SUPPORTED
#  define PNG_WARNING_PARAMETERS(p) png_warning_parameters p;
#else
#  define png_warning(s1,s2) ((void)(s1))
#  define png_chunk_warning(s1,s2) ((void)(s1))
#  define png_warning_parameter(p,number,string) ((void)0)
#  define png_warning_parameter_unsigned(p,number,format,value) ((void)0)
#  define png_warning_parameter_signed(p,number,format,value) ((void)0)
#  define png_formatted_warning(pp,p,message) ((void)(pp))
#  define PNG_WARNING_PARAMETERS(p)
#endif
#ifndef PNG_ERROR_TEXT_SUPPORTED
#  define png_error(s1,s2) png_err(s1)
#  define png_chunk_error(s1,s2) png_err(s1)
#  define png_fixed_error(s1,s2) png_err(s1)
#endif






#ifdef __cplusplus
#  define png_voidcast(type, value) static_cast<type>(value)
#else
#  define png_voidcast(type, value) (value)
#endif 

#ifndef PNG_EXTERN









#  define PNG_EXTERN extern
#endif





#ifdef PNG_FIXED_POINT_SUPPORTED
#  define PNGFAPI PNGAPI
#else
#  define PNGFAPI
#endif




#if defined(PNG_FLOATING_POINT_SUPPORTED) ||\
    defined(PNG_FLOATING_ARITHMETIC_SUPPORTED)
   






#  include <float.h>

#  if (defined(__MWERKS__) && defined(macintosh)) || defined(applec) || \
    defined(THINK_C) || defined(__SC__) || defined(TARGET_OS_MAC)
     



#    if !defined(__MATH_H__) && !defined(__MATH_H) && !defined(__cmath__)
#      include <fp.h>
#    endif
#  else
#    include <math.h>
#  endif
#  if defined(_AMIGA) && defined(__SASC) && defined(_M68881)
     


#    include <m68881.h>
#  endif
#endif


#if defined(__TURBOC__) && defined(__MSDOS__)
#  include <mem.h>
#  include <alloc.h>
#endif

#if defined(WIN32) || defined(_Windows) || defined(_WINDOWS) || \
    defined(_WIN32) || defined(__WIN32__)
#  include <windows.h>  
#endif







#ifndef PNG_ABORT
#  ifdef _WINDOWS_
#    define PNG_ABORT() ExitProcess(0)
#  else
#    define PNG_ABORT() abort()
#  endif
#endif

#ifdef USE_FAR_KEYWORD

#  define CHECK   1
#  define NOCHECK 0
#  define CVT_PTR(ptr) (png_far_to_near(png_ptr,ptr,CHECK))
#  define CVT_PTR_NOCHECK(ptr) (png_far_to_near(png_ptr,ptr,NOCHECK))
#  define png_strlen  _fstrlen
#  define png_memcmp  _fmemcmp    /* SJT: added */
#  define png_memcpy  _fmemcpy
#  define png_memset  _fmemset
#else
#  ifdef _WINDOWS_  
#    define CVT_PTR(ptr)         (ptr)
#    define CVT_PTR_NOCHECK(ptr) (ptr)
#    define png_strlen  lstrlenA
#    define png_memcmp  memcmp
#    define png_memcpy  CopyMemory
#    define png_memset  memset
#  else
#    define CVT_PTR(ptr)         (ptr)
#    define CVT_PTR_NOCHECK(ptr) (ptr)
#    define png_strlen  strlen
#    define png_memcmp  memcmp      /* SJT: added */
#    define png_memcpy  memcpy
#    define png_memset  memset
#  endif
#endif


#define PNG_ALIGN_NONE   0 /* do not use data alignment */
#define PNG_ALIGN_ALWAYS 1 /* assume unaligned accesses are OK */
#ifdef offsetof
#  define PNG_ALIGN_OFFSET 2 /* use offsetof to determine alignment */
#else
#  define PNG_ALIGN_OFFSET -1 /* prevent the use of this */
#endif
#define PNG_ALIGN_SIZE   3 /* use sizeof to determine alignment */

#ifndef PNG_ALIGN_TYPE
   



#  define PNG_ALIGN_TYPE PNG_ALIGN_SIZE
#endif

#if PNG_ALIGN_TYPE == PNG_ALIGN_SIZE
   





#  define png_alignof(type) (sizeof (type))
#else
#  if PNG_ALIGN_TYPE == PNG_ALIGN_OFFSET
#     define png_alignof(type) offsetof(struct{char c; type t;}, t)
#  else
#     if PNG_ALIGN_TYPE == PNG_ALIGN_ALWAYS
#        define png_alignof(type) (1)
#     endif
      
#  endif
#endif


#ifdef png_alignof
#  define png_isaligned(ptr, type)\
   ((((const char*)ptr-(const char*)0) & (png_alignof(type)-1)) == 0)
#else
#  define png_isaligned(ptr, type) 0
#endif















#define PNG_HAVE_IDAT               0x04

#define PNG_HAVE_IEND               0x10
#define PNG_HAVE_gAMA               0x20
#define PNG_HAVE_cHRM               0x40
#define PNG_HAVE_sRGB               0x80
#define PNG_HAVE_CHUNK_HEADER      0x100
#define PNG_WROTE_tIME             0x200
#define PNG_WROTE_INFO_BEFORE_PLTE 0x400
#define PNG_BACKGROUND_IS_GRAY     0x800
#define PNG_HAVE_PNG_SIGNATURE    0x1000
#define PNG_HAVE_CHUNK_AFTER_IDAT 0x2000 /* Have another chunk after IDAT */
#define PNG_HAVE_iCCP             0x4000
#ifdef PNG_APNG_SUPPORTED
#define PNG_HAVE_acTL             0x8000
#define PNG_HAVE_fcTL            0x10000
#endif


#define PNG_BGR                 0x0001
#define PNG_INTERLACE           0x0002
#define PNG_PACK                0x0004
#define PNG_SHIFT               0x0008
#define PNG_SWAP_BYTES          0x0010
#define PNG_INVERT_MONO         0x0020
#define PNG_QUANTIZE            0x0040
#define PNG_COMPOSE             0x0080     /* Was PNG_BACKGROUND */
#define PNG_BACKGROUND_EXPAND   0x0100
#define PNG_EXPAND_16           0x0200     /* Added to libpng 1.5.2 */
#define PNG_16_TO_8             0x0400     /* Becomes 'chop' in 1.5.4 */
#define PNG_RGBA                0x0800
#define PNG_EXPAND              0x1000
#define PNG_GAMMA               0x2000
#define PNG_GRAY_TO_RGB         0x4000
#define PNG_FILLER              0x8000
#define PNG_PACKSWAP           0x10000
#define PNG_SWAP_ALPHA         0x20000
#define PNG_STRIP_ALPHA        0x40000
#define PNG_INVERT_ALPHA       0x80000
#define PNG_USER_TRANSFORM    0x100000
#define PNG_RGB_TO_GRAY_ERR   0x200000
#define PNG_RGB_TO_GRAY_WARN  0x400000
#define PNG_RGB_TO_GRAY       0x600000 /* two bits, RGB_TO_GRAY_ERR|WARN */
#define PNG_ENCODE_ALPHA      0x800000 /* Added to libpng-1.5.4 */
#define PNG_ADD_ALPHA         0x1000000 /* Added to libpng-1.2.7 */
#define PNG_EXPAND_tRNS       0x2000000 /* Added to libpng-1.2.9 */
#define PNG_SCALE_16_TO_8     0x4000000 /* Added to libpng-1.5.4 */
                       
                       
                       
                       

#define PNG_STRUCT_PNG   0x0001
#define PNG_STRUCT_INFO  0x0002


#define PNG_WEIGHT_FACTOR (1<<(PNG_WEIGHT_SHIFT))
#define PNG_COST_FACTOR (1<<(PNG_COST_SHIFT))


#define PNG_FLAG_ZLIB_CUSTOM_STRATEGY     0x0001
#define PNG_FLAG_ZLIB_CUSTOM_LEVEL        0x0002
#define PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL    0x0004
#define PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS  0x0008
#define PNG_FLAG_ZLIB_CUSTOM_METHOD       0x0010
#define PNG_FLAG_ZLIB_FINISHED            0x0020
#define PNG_FLAG_ROW_INIT                 0x0040
#define PNG_FLAG_FILLER_AFTER             0x0080
#define PNG_FLAG_CRC_ANCILLARY_USE        0x0100
#define PNG_FLAG_CRC_ANCILLARY_NOWARN     0x0200
#define PNG_FLAG_CRC_CRITICAL_USE         0x0400
#define PNG_FLAG_CRC_CRITICAL_IGNORE      0x0800
#define PNG_FLAG_ASSUME_sRGB              0x1000  /* Added to libpng-1.5.4 */
#define PNG_FLAG_OPTIMIZE_ALPHA           0x2000  /* Added to libpng-1.5.4 */
#define PNG_FLAG_DETECT_UNINITIALIZED     0x4000  /* Added to libpng-1.5.4 */
#define PNG_FLAG_KEEP_UNKNOWN_CHUNKS      0x8000
#define PNG_FLAG_KEEP_UNSAFE_CHUNKS       0x10000
#define PNG_FLAG_LIBRARY_MISMATCH         0x20000
#define PNG_FLAG_STRIP_ERROR_NUMBERS      0x40000
#define PNG_FLAG_STRIP_ERROR_TEXT         0x80000
#define PNG_FLAG_MALLOC_NULL_MEM_OK       0x100000
                                  
                                  
#define PNG_FLAG_BENIGN_ERRORS_WARN       0x800000  /* Added to libpng-1.4.0 */
#define PNG_FLAG_ZTXT_CUSTOM_STRATEGY    0x1000000  /* 5 lines added */
#define PNG_FLAG_ZTXT_CUSTOM_LEVEL       0x2000000  /* to libpng-1.5.4 */
#define PNG_FLAG_ZTXT_CUSTOM_MEM_LEVEL   0x4000000
#define PNG_FLAG_ZTXT_CUSTOM_WINDOW_BITS 0x8000000
#define PNG_FLAG_ZTXT_CUSTOM_METHOD      0x10000000
                                  
                                  

#define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)

#define PNG_FLAG_CRC_CRITICAL_MASK  (PNG_FLAG_CRC_CRITICAL_USE | \
                                     PNG_FLAG_CRC_CRITICAL_IGNORE)

#define PNG_FLAG_CRC_MASK           (PNG_FLAG_CRC_ANCILLARY_MASK | \
                                     PNG_FLAG_CRC_CRITICAL_MASK)









#ifndef ZLIB_IO_MAX
#  define ZLIB_IO_MAX ((uInt)-1)
#endif



#define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))


#define PNG_ROWBYTES(pixel_bits, width) \
    ((pixel_bits) >= 8 ? \
    ((png_size_t)(width) * (((png_size_t)(pixel_bits)) >> 3)) : \
    (( ((png_size_t)(width) * ((png_size_t)(pixel_bits))) + 7) >> 3) )






#define PNG_OUT_OF_RANGE(value, ideal, delta) \
   ( (value) < (ideal)-(delta) || (value) > (ideal)+(delta) )





#ifdef PNG_FLOATING_POINT_SUPPORTED







#define png_float(png_ptr, fixed, s) (.00001 * (fixed))

















#ifdef PNG_FIXED_POINT_MACRO_SUPPORTED
#define png_fixed(png_ptr, fp, s) ((fp) <= 21474 && (fp) >= -21474 ?\
    ((png_fixed_point)(100000 * (fp))) : (png_fixed_error(png_ptr, s),0))
#else
PNG_EXTERN png_fixed_point png_fixed PNGARG((png_structp png_ptr, double fp,
   png_const_charp text));
#endif
#endif














#define PNG_32b(b,s) ((png_uint_32)(b) << (s))
#define PNG_CHUNK(b1,b2,b3,b4) \
   (PNG_32b(b1,24) | PNG_32b(b2,16) | PNG_32b(b3,8) | PNG_32b(b4,0))

#define png_IHDR PNG_CHUNK( 73,  72,  68,  82)
#define png_IDAT PNG_CHUNK( 73,  68,  65,  84)
#define png_IEND PNG_CHUNK( 73,  69,  78,  68)
#define png_PLTE PNG_CHUNK( 80,  76,  84,  69)
#define png_bKGD PNG_CHUNK( 98,  75,  71,  68)
#define png_cHRM PNG_CHUNK( 99,  72,  82,  77)
#define png_gAMA PNG_CHUNK(103,  65,  77,  65)
#define png_hIST PNG_CHUNK(104,  73,  83,  84)
#define png_iCCP PNG_CHUNK(105,  67,  67,  80)
#define png_iTXt PNG_CHUNK(105,  84,  88, 116)
#define png_oFFs PNG_CHUNK(111,  70,  70, 115)
#define png_pCAL PNG_CHUNK(112,  67,  65,  76)
#define png_sCAL PNG_CHUNK(115,  67,  65,  76)
#define png_pHYs PNG_CHUNK(112,  72,  89, 115)
#define png_sBIT PNG_CHUNK(115,  66,  73,  84)
#define png_sPLT PNG_CHUNK(115,  80,  76,  84)
#define png_sRGB PNG_CHUNK(115,  82,  71,  66)
#define png_sTER PNG_CHUNK(115,  84,  69,  82)
#define png_tEXt PNG_CHUNK(116,  69,  88, 116)
#define png_tIME PNG_CHUNK(116,  73,  77,  69)
#define png_tRNS PNG_CHUNK(116,  82,  78,  83)
#define png_zTXt PNG_CHUNK(122,  84,  88, 116)

#ifdef PNG_APNG_SUPPORTED
#define png_acTL PNG_CHUNK( 97,  99,  84,  76)
#define png_fcTL PNG_CHUNK(102,  99,  84,  76)
#define png_fdAT PNG_CHUNK(102, 100,  65,  84)


#define PNG_FIRST_FRAME_HIDDEN       0x0001
#define PNG_APNG_APP                 0x0002
#endif




#define PNG_CHUNK_FROM_STRING(s)\
   PNG_CHUNK(0xff&(s)[0], 0xff&(s)[1], 0xff&(s)[2], 0xff&(s)[3])





#define PNG_STRING_FROM_CHUNK(s,c)\
   (void)(((char*)(s))[0]=(char)((c)>>24), ((char*)(s))[1]=(char)((c)>>16),\
   ((char*)(s))[2]=(char)((c)>>8), ((char*)(s))[3]=(char)((c)))


#define PNG_CSTRING_FROM_CHUNK(s,c)\
   (void)(PNG_STRING_FROM_CHUNK(s,c), ((char*)(s))[4] = 0)


#define PNG_CHUNK_ANCILLIARY(c)   (1 & ((c) >> 29))
#define PNG_CHUNK_CRITICAL(c)     (!PNG_CHUNK_ANCILLIARY(c))
#define PNG_CHUNK_PRIVATE(c)      (1 & ((c) >> 21))
#define PNG_CHUNK_RESERVED(c)     (1 & ((c) >> 13))
#define PNG_CHUNK_SAFE_TO_COPY(c) (1 & ((c) >>  5))


#define PNG_GAMMA_MAC_OLD 151724  /* Assume '1.8' is really 2.2/1.45! */
#define PNG_GAMMA_MAC_INVERSE 65909
#define PNG_GAMMA_sRGB_INVERSE 45455



#ifdef __cplusplus
extern "C" {
#endif










PNG_EXTERN int png_user_version_check(png_structp png_ptr,
   png_const_charp user_png_ver);


PNG_EXTERN PNG_FUNCTION(png_voidp,png_create_struct,PNGARG((int type)),
   PNG_ALLOCATED);


PNG_EXTERN void png_destroy_struct PNGARG((png_voidp struct_ptr));

PNG_EXTERN PNG_FUNCTION(png_voidp,png_create_struct_2,
   PNGARG((int type, png_malloc_ptr malloc_fn, png_voidp mem_ptr)),
   PNG_ALLOCATED);
PNG_EXTERN void png_destroy_struct_2 PNGARG((png_voidp struct_ptr,
    png_free_ptr free_fn, png_voidp mem_ptr));


PNG_EXTERN void png_info_destroy PNGARG((png_structp png_ptr,
    png_infop info_ptr));


PNG_EXTERN PNG_FUNCTION(voidpf,png_zalloc,PNGARG((voidpf png_ptr, uInt items,
   uInt size)),PNG_ALLOCATED);


PNG_EXTERN void png_zfree PNGARG((voidpf png_ptr, voidpf ptr));






PNG_EXTERN void PNGCBAPI png_default_read_data PNGARG((png_structp png_ptr,
    png_bytep data, png_size_t length));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void PNGCBAPI png_push_fill_buffer PNGARG((png_structp png_ptr,
    png_bytep buffer, png_size_t length));
#endif

PNG_EXTERN void PNGCBAPI png_default_write_data PNGARG((png_structp png_ptr,
    png_bytep data, png_size_t length));

#ifdef PNG_WRITE_FLUSH_SUPPORTED
#  ifdef PNG_STDIO_SUPPORTED
PNG_EXTERN void PNGCBAPI png_default_flush PNGARG((png_structp png_ptr));
#  endif
#endif


PNG_EXTERN void png_reset_crc PNGARG((png_structp png_ptr));


PNG_EXTERN void png_write_data PNGARG((png_structp png_ptr,
    png_const_bytep data, png_size_t length));


PNG_EXTERN void png_read_sig PNGARG((png_structp png_ptr, png_infop info_ptr));


PNG_EXTERN png_uint_32 png_read_chunk_header PNGARG((png_structp png_ptr));


PNG_EXTERN void png_read_data PNGARG((png_structp png_ptr, png_bytep data,
    png_size_t length));


PNG_EXTERN void png_crc_read PNGARG((png_structp png_ptr, png_bytep buf,
    png_size_t length));


#if defined(PNG_READ_COMPRESSED_TEXT_SUPPORTED)
PNG_EXTERN void png_decompress_chunk PNGARG((png_structp png_ptr,
    int comp_type, png_size_t chunklength, png_size_t prefix_length,
    png_size_t *data_length));
#endif


PNG_EXTERN int png_crc_finish PNGARG((png_structp png_ptr, png_uint_32 skip));


PNG_EXTERN int png_crc_error PNGARG((png_structp png_ptr));





PNG_EXTERN void png_calculate_crc PNGARG((png_structp png_ptr,
    png_const_bytep ptr, png_size_t length));

#ifdef PNG_WRITE_FLUSH_SUPPORTED
PNG_EXTERN void png_flush PNGARG((png_structp png_ptr));
#endif






PNG_EXTERN void png_write_IHDR PNGARG((png_structp png_ptr, png_uint_32 width,
    png_uint_32 height,
    int bit_depth, int color_type, int compression_method, int filter_method,
    int interlace_method));

PNG_EXTERN void png_write_PLTE PNGARG((png_structp png_ptr,
    png_const_colorp palette, png_uint_32 num_pal));

PNG_EXTERN void png_write_IDAT PNGARG((png_structp png_ptr, png_bytep data,
    png_size_t length));

PNG_EXTERN void png_write_IEND PNGARG((png_structp png_ptr));

#ifdef PNG_WRITE_gAMA_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_write_gAMA PNGARG((png_structp png_ptr, double file_gamma));
#  endif
PNG_EXTERN void png_write_gAMA_fixed PNGARG((png_structp png_ptr,
    png_fixed_point file_gamma));
#endif

#ifdef PNG_WRITE_sBIT_SUPPORTED
PNG_EXTERN void png_write_sBIT PNGARG((png_structp png_ptr,
    png_const_color_8p sbit, int color_type));
#endif

#ifdef PNG_WRITE_cHRM_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_write_cHRM PNGARG((png_structp png_ptr,
    double white_x, double white_y,
    double red_x, double red_y, double green_x, double green_y,
    double blue_x, double blue_y));
#  endif
PNG_EXTERN void png_write_cHRM_fixed PNGARG((png_structp png_ptr,
    png_fixed_point int_white_x, png_fixed_point int_white_y,
    png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
    int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
    png_fixed_point int_blue_y));
#endif

#ifdef PNG_WRITE_sRGB_SUPPORTED
PNG_EXTERN void png_write_sRGB PNGARG((png_structp png_ptr,
    int intent));
#endif

#ifdef PNG_WRITE_iCCP_SUPPORTED
PNG_EXTERN void png_write_iCCP PNGARG((png_structp png_ptr,
    png_const_charp name, int compression_type,
    png_const_charp profile, int proflen));
   
#endif

#ifdef PNG_WRITE_sPLT_SUPPORTED
PNG_EXTERN void png_write_sPLT PNGARG((png_structp png_ptr,
    png_const_sPLT_tp palette));
#endif

#ifdef PNG_WRITE_tRNS_SUPPORTED
PNG_EXTERN void png_write_tRNS PNGARG((png_structp png_ptr,
    png_const_bytep trans, png_const_color_16p values, int number,
    int color_type));
#endif

#ifdef PNG_WRITE_bKGD_SUPPORTED
PNG_EXTERN void png_write_bKGD PNGARG((png_structp png_ptr,
    png_const_color_16p values, int color_type));
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED
PNG_EXTERN void png_write_hIST PNGARG((png_structp png_ptr,
    png_const_uint_16p hist, int num_hist));
#endif


#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED) || \
    defined(PNG_WRITE_iCCP_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
PNG_EXTERN png_size_t png_check_keyword PNGARG((png_structp png_ptr,
    png_const_charp key, png_charpp new_key));
#endif

#ifdef PNG_WRITE_tEXt_SUPPORTED
PNG_EXTERN void png_write_tEXt PNGARG((png_structp png_ptr, png_const_charp key,
    png_const_charp text, png_size_t text_len));
#endif

#ifdef PNG_WRITE_zTXt_SUPPORTED
PNG_EXTERN void png_write_zTXt PNGARG((png_structp png_ptr, png_const_charp key,
    png_const_charp text, png_size_t text_len, int compression));
#endif

#ifdef PNG_WRITE_iTXt_SUPPORTED
PNG_EXTERN void png_write_iTXt PNGARG((png_structp png_ptr,
    int compression, png_const_charp key, png_const_charp lang,
    png_const_charp lang_key, png_const_charp text));
#endif

#ifdef PNG_TEXT_SUPPORTED  
PNG_EXTERN int png_set_text_2 PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_const_textp text_ptr, int num_text));
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED
PNG_EXTERN void png_write_oFFs PNGARG((png_structp png_ptr,
    png_int_32 x_offset, png_int_32 y_offset, int unit_type));
#endif

#ifdef PNG_WRITE_pCAL_SUPPORTED
PNG_EXTERN void png_write_pCAL PNGARG((png_structp png_ptr, png_charp purpose,
    png_int_32 X0, png_int_32 X1, int type, int nparams,
    png_const_charp units, png_charpp params));
#endif

#ifdef PNG_WRITE_pHYs_SUPPORTED
PNG_EXTERN void png_write_pHYs PNGARG((png_structp png_ptr,
    png_uint_32 x_pixels_per_unit, png_uint_32 y_pixels_per_unit,
    int unit_type));
#endif

#ifdef PNG_WRITE_tIME_SUPPORTED
PNG_EXTERN void png_write_tIME PNGARG((png_structp png_ptr,
    png_const_timep mod_time));
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED
PNG_EXTERN void png_write_sCAL_s PNGARG((png_structp png_ptr,
    int unit, png_const_charp width, png_const_charp height));
#endif


PNG_EXTERN void png_write_finish_row PNGARG((png_structp png_ptr));


PNG_EXTERN void png_write_start_row PNGARG((png_structp png_ptr));
























#ifndef PNG_USE_COMPILE_TIME_MASKS
#  define PNG_USE_COMPILE_TIME_MASKS 1
#endif
PNG_EXTERN void png_combine_row PNGARG((png_structp png_ptr, png_bytep row,
    int display));

#ifdef PNG_READ_INTERLACING_SUPPORTED






PNG_EXTERN void png_do_read_interlace PNGARG((png_row_infop row_info,
    png_bytep row, int pass, png_uint_32 transformations));
#endif



#ifdef PNG_WRITE_INTERLACING_SUPPORTED

PNG_EXTERN void png_do_write_interlace PNGARG((png_row_infop row_info,
    png_bytep row, int pass));
#endif




PNG_EXTERN void png_read_filter_row PNGARG((png_structp pp, png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row, int filter));

PNG_EXTERN void png_read_filter_row_up_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));
PNG_EXTERN void png_read_filter_row_sub3_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));
PNG_EXTERN void png_read_filter_row_sub4_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));
PNG_EXTERN void png_read_filter_row_avg3_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));
PNG_EXTERN void png_read_filter_row_avg4_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));
PNG_EXTERN void png_read_filter_row_paeth3_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));
PNG_EXTERN void png_read_filter_row_paeth4_neon PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row));


PNG_EXTERN void png_write_find_filter PNGARG((png_structp png_ptr,
    png_row_infop row_info));


PNG_EXTERN void png_read_finish_row PNGARG((png_structp png_ptr));


PNG_EXTERN void png_read_start_row PNGARG((png_structp png_ptr));

#ifdef PNG_READ_TRANSFORMS_SUPPORTED

PNG_EXTERN void png_read_transform_info PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#endif


#ifdef PNG_READ_FILLER_SUPPORTED
PNG_EXTERN void png_do_read_filler PNGARG((png_row_infop row_info,
    png_bytep row, png_uint_32 filler, png_uint_32 flags));
#endif

#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
PNG_EXTERN void png_do_read_swap_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_WRITE_SWAP_ALPHA_SUPPORTED
PNG_EXTERN void png_do_write_swap_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
PNG_EXTERN void png_do_read_invert_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
PNG_EXTERN void png_do_write_invert_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED) || \
    defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_strip_channel PNGARG((png_row_infop row_info,
    png_bytep row, int at_start));
#endif

#ifdef PNG_16BIT_SUPPORTED
#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
PNG_EXTERN void png_do_swap PNGARG((png_row_infop row_info,
    png_bytep row));
#endif
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || \
    defined(PNG_WRITE_PACKSWAP_SUPPORTED)
PNG_EXTERN void png_do_packswap PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
PNG_EXTERN int png_do_rgb_to_gray PNGARG((png_structp png_ptr,
    png_row_infop row_info, png_bytep row));
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
PNG_EXTERN void png_do_gray_to_rgb PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_PACK_SUPPORTED
PNG_EXTERN void png_do_unpack PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_SHIFT_SUPPORTED
PNG_EXTERN void png_do_unshift PNGARG((png_row_infop row_info,
    png_bytep row, png_const_color_8p sig_bits));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
PNG_EXTERN void png_do_invert PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
PNG_EXTERN void png_do_scale_16_to_8 PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
PNG_EXTERN void png_do_chop PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED
PNG_EXTERN void png_do_quantize PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep palette_lookup,
    png_const_bytep quantize_lookup));

#  ifdef PNG_CORRECT_PALETTE_SUPPORTED
PNG_EXTERN void png_correct_palette PNGARG((png_structp png_ptr,
    png_colorp palette, int num_palette));
#  endif
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
PNG_EXTERN void png_do_bgr PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_WRITE_PACK_SUPPORTED
PNG_EXTERN void png_do_pack PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 bit_depth));
#endif

#ifdef PNG_WRITE_SHIFT_SUPPORTED
PNG_EXTERN void png_do_shift PNGARG((png_row_infop row_info,
    png_bytep row, png_const_color_8p bit_depth));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
    defined(PNG_READ_ALPHA_MODE_SUPPORTED)
PNG_EXTERN void png_do_compose PNGARG((png_row_infop row_info,
    png_bytep row, png_structp png_ptr));
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
PNG_EXTERN void png_do_gamma PNGARG((png_row_infop row_info,
    png_bytep row, png_structp png_ptr));
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
PNG_EXTERN void png_do_encode_alpha PNGARG((png_row_infop row_info,
   png_bytep row, png_structp png_ptr));
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED
PNG_EXTERN void png_do_expand_palette PNGARG((png_row_infop row_info,
    png_bytep row, png_const_colorp palette, png_const_bytep trans,
    int num_trans));
PNG_EXTERN void png_do_expand PNGARG((png_row_infop row_info,
    png_bytep row, png_const_color_16p trans_color));
#endif

#ifdef PNG_READ_EXPAND_16_SUPPORTED
PNG_EXTERN void png_do_expand_16 PNGARG((png_row_infop row_info,
    png_bytep row));
#endif






PNG_EXTERN void png_handle_IHDR PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
PNG_EXTERN void png_handle_PLTE PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
PNG_EXTERN void png_handle_IEND PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));

#ifdef PNG_READ_bKGD_SUPPORTED
PNG_EXTERN void png_handle_bKGD PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_cHRM_SUPPORTED
PNG_EXTERN void png_handle_cHRM PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_gAMA_SUPPORTED
PNG_EXTERN void png_handle_gAMA PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_hIST_SUPPORTED
PNG_EXTERN void png_handle_hIST PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_iCCP_SUPPORTED
PNG_EXTERN void png_handle_iCCP PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif 

#ifdef PNG_READ_iTXt_SUPPORTED
PNG_EXTERN void png_handle_iTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
PNG_EXTERN void png_handle_oFFs PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
PNG_EXTERN void png_handle_pCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
PNG_EXTERN void png_handle_pHYs PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
PNG_EXTERN void png_handle_sBIT PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_sCAL_SUPPORTED
PNG_EXTERN void png_handle_sCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_sPLT_SUPPORTED
PNG_EXTERN void png_handle_sPLT PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif 

#ifdef PNG_READ_sRGB_SUPPORTED
PNG_EXTERN void png_handle_sRGB PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_tEXt_SUPPORTED
PNG_EXTERN void png_handle_tEXt PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_tIME_SUPPORTED
PNG_EXTERN void png_handle_tIME PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_tRNS_SUPPORTED
PNG_EXTERN void png_handle_tRNS PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_zTXt_SUPPORTED
PNG_EXTERN void png_handle_zTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

PNG_EXTERN void png_handle_unknown PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));

PNG_EXTERN void png_check_chunk_name PNGARG((png_structp png_ptr,
    png_uint_32 chunk_name));

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED



PNG_EXTERN int png_chunk_unknown_handling PNGARG((png_structp png_ptr,
    png_uint_32 chunk_name));
#endif


#ifdef PNG_READ_TRANSFORMS_SUPPORTED
PNG_EXTERN void png_do_read_transformations PNGARG((png_structp png_ptr,
   png_row_infop row_info));
#endif
#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
PNG_EXTERN void png_do_write_transformations PNGARG((png_structp png_ptr,
   png_row_infop row_info));
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
PNG_EXTERN void png_init_read_transformations PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void png_push_read_chunk PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_push_read_sig PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_push_check_crc PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_crc_skip PNGARG((png_structp png_ptr,
    png_uint_32 length));
PNG_EXTERN void png_push_crc_finish PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_save_buffer PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_restore_buffer PNGARG((png_structp png_ptr,
    png_bytep buffer, png_size_t buffer_length));
PNG_EXTERN void png_push_read_IDAT PNGARG((png_structp png_ptr));
PNG_EXTERN void png_process_IDAT_data PNGARG((png_structp png_ptr,
    png_bytep buffer, png_size_t buffer_length));
PNG_EXTERN void png_push_process_row PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_handle_unknown PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_have_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_have_end PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_have_row PNGARG((png_structp png_ptr, png_bytep row));
PNG_EXTERN void png_push_read_end PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_process_some_data PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_read_push_finish_row PNGARG((png_structp png_ptr));
#  ifdef PNG_READ_tEXt_SUPPORTED
PNG_EXTERN void png_push_handle_tEXt PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_tEXt PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#  endif
#  ifdef PNG_READ_zTXt_SUPPORTED
PNG_EXTERN void png_push_handle_zTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_zTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#  endif
#  ifdef PNG_READ_iTXt_SUPPORTED
PNG_EXTERN void png_push_handle_iTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_iTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#  endif

#endif 

#ifdef PNG_MNG_FEATURES_SUPPORTED
PNG_EXTERN void png_do_read_intrapixel PNGARG((png_row_infop row_info,
    png_bytep row));
PNG_EXTERN void png_do_write_intrapixel PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_APNG_SUPPORTED
PNG_EXTERN void png_ensure_fcTL_is_valid PNGARG((png_structp png_ptr,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den,
   png_byte dispose_op, png_byte blend_op));

#ifdef PNG_READ_APNG_SUPPORTED
PNG_EXTERN void png_handle_acTL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_fcTL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_fdAT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_have_info PNGARG((png_structp png_ptr, png_infop info_ptr));
PNG_EXTERN void png_ensure_sequence_number PNGARG((png_structp png_ptr,
   png_uint_32 length));
PNG_EXTERN void png_read_reset PNGARG((png_structp png_ptr));
PNG_EXTERN void png_read_reinit PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void png_progressive_read_reset PNGARG((png_structp png_ptr));
#endif 
#endif 

#ifdef PNG_WRITE_APNG_SUPPORTED
PNG_EXTERN void png_write_acTL PNGARG((png_structp png_ptr,
   png_uint_32 num_frames, png_uint_32 num_plays));
PNG_EXTERN void png_write_fcTL PNGARG((png_structp png_ptr,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den,
   png_byte dispose_op, png_byte blend_op));
PNG_EXTERN void png_write_reset PNGARG((png_structp png_ptr));
PNG_EXTERN void png_write_reinit PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height));
#endif 
#endif 


#ifdef PNG_CHECK_cHRM_SUPPORTED
PNG_EXTERN int png_check_cHRM_fixed PNGARG((png_structp png_ptr,
    png_fixed_point int_white_x, png_fixed_point int_white_y,
    png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
    int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
    png_fixed_point int_blue_y));
#endif

#ifdef PNG_CHECK_cHRM_SUPPORTED


PNG_EXTERN void png_64bit_product PNGARG((long v1, long v2,
    unsigned long *hi_product, unsigned long *lo_product));
#endif

#ifdef PNG_cHRM_SUPPORTED

typedef struct png_xy
{
   png_fixed_point redx, redy;
   png_fixed_point greenx, greeny;
   png_fixed_point bluex, bluey;
   png_fixed_point whitex, whitey;
} png_xy;

typedef struct png_XYZ
{
   png_fixed_point redX, redY, redZ;
   png_fixed_point greenX, greenY, greenZ;
   png_fixed_point blueX, blueY, blueZ;
} png_XYZ;







PNG_EXTERN int png_xy_from_XYZ PNGARG((png_xy *xy, png_XYZ XYZ));
PNG_EXTERN int png_XYZ_from_xy PNGARG((png_XYZ *XYZ, png_xy xy));
PNG_EXTERN int png_XYZ_from_xy_checked PNGARG((png_structp png_ptr,
   png_XYZ *XYZ, png_xy xy));
#endif


PNG_EXTERN void png_check_IHDR PNGARG((png_structp png_ptr,
    png_uint_32 width, png_uint_32 height, int bit_depth,
    int color_type, int interlace_type, int compression_type,
    int filter_type));


#if defined(PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED) || \
    defined(PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED)
PNG_EXTERN void png_do_check_palette_indexes PNGARG((png_structp png_ptr,
    png_row_infop row_info));
#endif


PNG_EXTERN void png_read_destroy PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_infop end_info_ptr));


PNG_EXTERN void png_write_destroy PNGARG((png_structp png_ptr));

#ifdef USE_FAR_KEYWORD  
PNG_EXTERN void *png_far_to_near PNGARG((png_structp png_ptr, png_voidp ptr,
    int check));
#endif 

#if defined(PNG_FLOATING_POINT_SUPPORTED) && defined(PNG_ERROR_TEXT_SUPPORTED)
PNG_EXTERN PNG_FUNCTION(void, png_fixed_error, (png_structp png_ptr,
   png_const_charp name),PNG_NORETURN);
#endif





PNG_EXTERN size_t png_safecat(png_charp buffer, size_t bufsize, size_t pos,
    png_const_charp string);




#if defined(PNG_WARNINGS_SUPPORTED) || defined(PNG_TIME_RFC1123_SUPPORTED)





PNG_EXTERN png_charp png_format_number(png_const_charp start, png_charp end,
   int format, png_alloc_size_t number);


#define PNG_FORMAT_NUMBER(buffer,format,number) \
   png_format_number(buffer, buffer + (sizeof buffer), format, number)


#define PNG_NUMBER_BUFFER_SIZE 24




#define PNG_NUMBER_FORMAT_u     1 /* chose unsigned API! */
#define PNG_NUMBER_FORMAT_02u   2
#define PNG_NUMBER_FORMAT_d     1 /* chose signed API! */
#define PNG_NUMBER_FORMAT_02d   2
#define PNG_NUMBER_FORMAT_x     3
#define PNG_NUMBER_FORMAT_02x   4
#define PNG_NUMBER_FORMAT_fixed 5 /* choose the signed API */
#endif

#ifdef PNG_WARNINGS_SUPPORTED

#  define PNG_WARNING_PARAMETER_SIZE 32
#  define PNG_WARNING_PARAMETER_COUNT 8




typedef char png_warning_parameters[PNG_WARNING_PARAMETER_COUNT][
   PNG_WARNING_PARAMETER_SIZE];

PNG_EXTERN void png_warning_parameter(png_warning_parameters p, int number,
    png_const_charp string);
    


PNG_EXTERN void png_warning_parameter_unsigned(png_warning_parameters p,
    int number, int format, png_alloc_size_t value);
    


PNG_EXTERN void png_warning_parameter_signed(png_warning_parameters p,
    int number, int format, png_int_32 value);

PNG_EXTERN void png_formatted_warning(png_structp png_ptr,
    png_warning_parameters p, png_const_charp message);
    



#endif




#ifdef PNG_sCAL_SUPPORTED





#define PNG_sCAL_MAX_DIGITS (PNG_sCAL_PRECISION+1/*.*/+1/*E*/+10/*exponent*/)
#endif

#ifdef PNG_sCAL_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_ascii_from_fp PNGARG((png_structp png_ptr, png_charp ascii,
    png_size_t size, double fp, unsigned int precision));
#endif 

#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXTERN void png_ascii_from_fixed PNGARG((png_structp png_ptr,
    png_charp ascii, png_size_t size, png_fixed_point fp));
#endif 
#endif 

#if defined(PNG_sCAL_SUPPORTED) || defined(PNG_pCAL_SUPPORTED)






























#define PNG_FP_INTEGER    0  /* before or in integer */
#define PNG_FP_FRACTION   1  /* before or in fraction */
#define PNG_FP_EXPONENT   2  /* before or in exponent */
#define PNG_FP_STATE      3  /* mask for the above */
#define PNG_FP_SAW_SIGN   4  /* Saw +/- in current state */
#define PNG_FP_SAW_DIGIT  8  /* Saw a digit in current state */
#define PNG_FP_SAW_DOT   16  /* Saw a dot in current state */
#define PNG_FP_SAW_E     32  /* Saw an E (or e) in current state */
#define PNG_FP_SAW_ANY   60  /* Saw any of the above 4 */



#define PNG_FP_WAS_VALID 64  /* Preceding substring is a valid fp number */
#define PNG_FP_NEGATIVE 128  /* A negative number, including "-0" */
#define PNG_FP_NONZERO  256  /* A non-zero value */
#define PNG_FP_STICKY   448  /* The above three flags */




#define PNG_FP_INVALID  512  /* Available for callers as a distinct value */




#define PNG_FP_MAYBE      0  /* The number may be valid in the future */
#define PNG_FP_OK         1  /* The number is valid */






#define PNG_FP_NZ_MASK (PNG_FP_SAW_DIGIT | PNG_FP_NEGATIVE | PNG_FP_NONZERO)
   
#define PNG_FP_Z_MASK (PNG_FP_SAW_DIGIT | PNG_FP_NONZERO)
   
   
#define PNG_FP_IS_ZERO(state) (((state) & PNG_FP_Z_MASK) == PNG_FP_SAW_DIGIT)
#define PNG_FP_IS_POSITIVE(state) (((state) & PNG_FP_NZ_MASK) == PNG_FP_Z_MASK)
#define PNG_FP_IS_NEGATIVE(state) (((state) & PNG_FP_NZ_MASK) == PNG_FP_NZ_MASK)
















PNG_EXTERN int png_check_fp_number PNGARG((png_const_charp string,
    png_size_t size, int *statep, png_size_tp whereami));







PNG_EXTERN int png_check_fp_string PNGARG((png_const_charp string,
    png_size_t size));
#endif 

#if defined(PNG_READ_GAMMA_SUPPORTED) ||\
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG_READ_pHYs_SUPPORTED)






PNG_EXTERN int png_muldiv PNGARG((png_fixed_point_p res, png_fixed_point a,
    png_int_32 multiplied_by, png_int_32 divided_by));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_INCH_CONVERSIONS_SUPPORTED)

PNG_EXTERN png_fixed_point png_muldiv_warn PNGARG((png_structp png_ptr,
    png_fixed_point a, png_int_32 multiplied_by, png_int_32 divided_by));
#endif

#if (defined PNG_READ_GAMMA_SUPPORTED) || (defined PNG_cHRM_SUPPORTED)




PNG_EXTERN png_fixed_point png_reciprocal PNGARG((png_fixed_point a));





PNG_EXTERN png_fixed_point png_reciprocal2 PNGARG((png_fixed_point a,
    png_fixed_point b));
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED







PNG_EXTERN png_uint_16 png_gamma_correct PNGARG((png_structp png_ptr,
    unsigned int value, png_fixed_point gamma_value));
PNG_EXTERN int png_gamma_significant PNGARG((png_fixed_point gamma_value));
PNG_EXTERN png_uint_16 png_gamma_16bit_correct PNGARG((unsigned int value,
    png_fixed_point gamma_value));
PNG_EXTERN png_byte png_gamma_8bit_correct PNGARG((unsigned int value,
    png_fixed_point gamma_value));
PNG_EXTERN void png_destroy_gamma_table(png_structp png_ptr);
PNG_EXTERN void png_build_gamma_table PNGARG((png_structp png_ptr,
    int bit_depth));
#endif




#ifndef PNG_FIXED_POINT_SUPPORTED
#ifdef PNG_cHRM_SUPPORTED
PNG_EXTERN png_uint_32 png_get_cHRM_XYZ_fixed PNGARG(
    (png_structp png_ptr, png_const_infop info_ptr,
    png_fixed_point *int_red_X, png_fixed_point *int_red_Y,
    png_fixed_point *int_red_Z, png_fixed_point *int_green_X,
    png_fixed_point *int_green_Y, png_fixed_point *int_green_Z,
    png_fixed_point *int_blue_X, png_fixed_point *int_blue_Y,
    png_fixed_point *int_blue_Z));
PNG_EXTERN void png_set_cHRM_XYZ_fixed PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_fixed_point int_red_X, png_fixed_point int_red_Y,
    png_fixed_point int_red_Z, png_fixed_point int_green_X,
    png_fixed_point int_green_Y, png_fixed_point int_green_Z,
    png_fixed_point int_blue_X, png_fixed_point int_blue_Y,
    png_fixed_point int_blue_Z));
PNG_EXTERN void png_set_cHRM_fixed PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_fixed_point int_white_x,
    png_fixed_point int_white_y, png_fixed_point int_red_x,
    png_fixed_point int_red_y, png_fixed_point int_green_x,
    png_fixed_point int_green_y, png_fixed_point int_blue_x,
    png_fixed_point int_blue_y));
#endif

#ifdef PNG_gAMA_SUPPORTED
PNG_EXTERN png_uint_32 png_get_gAMA_fixed PNGARG(
    (png_const_structp png_ptr, png_const_infop info_ptr,
    png_fixed_point *int_file_gamma));
PNG_EXTERN void png_set_gAMA_fixed PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_fixed_point int_file_gamma));
#endif

#ifdef PNG_READ_BACKGROUND_SUPPORTED
PNG_EXTERN void png_set_background_fixed PNGARG((png_structp png_ptr,
    png_const_color_16p background_color, int background_gamma_code,
    int need_expand, png_fixed_point background_gamma));
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
PNG_EXTERN void png_set_alpha_mode_fixed PNGARG((png_structp png_ptr,
    int mode, png_fixed_point output_gamma));
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
PNG_EXTERN void png_set_gamma_fixed PNGARG((png_structp png_ptr,
    png_fixed_point screen_gamma, png_fixed_point override_file_gamma));
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
PNG_EXTERN void png_set_rgb_to_gray_fixed PNGARG((png_structp png_ptr,
    int error_action, png_fixed_point red, png_fixed_point green));
#endif
#endif 

#ifdef PNG_FILTER_OPTIMIZATIONS
PNG_EXTERN void PNG_FILTER_OPTIMIZATIONS(png_structp png_ptr, unsigned int bpp);
   




#endif



#include "pngdebug.h"

#ifdef __cplusplus
}
#endif

#endif
