





















#ifndef PNGPRIV_H
#define PNGPRIV_H













#define _POSIX_SOURCE 1 /* Just the POSIX 1003.1 and C89 APIs */

#ifndef PNG_VERSION_INFO_ONLY

#  include <stdlib.h>
#  include <string.h>
#endif

#define PNGLIB_BUILD










#if defined(HAVE_CONFIG_H) && !defined(PNG_NO_CONFIG_H)
#  include <config.h>

   
#  define PNG_RESTRICT restrict
#endif






#ifndef PNGLCONF_H
#  include "pnglibconf.h"
#endif


#if defined(PNG_PREFIX) && !defined(PNGPREFIX_H)
#  include "pngprefix.h"
#endif

#ifdef PNG_USER_CONFIG
#  include "pngusr.h"
   
#  ifndef PNG_USER_PRIVATEBUILD
#    define PNG_USER_PRIVATEBUILD "Custom libpng build"
#  endif
#  ifndef PNG_USER_DLLFNAME_POSTFIX
#    define PNG_USER_DLLFNAME_POSTFIX "Cb"
#  endif
#endif




















#ifndef PNG_ARM_NEON_OPT
   














#  if (defined(__ARM_NEON__) || defined(__ARM_NEON)) && \
   defined(PNG_ALIGNED_MEMORY_SUPPORTED)
#     define PNG_ARM_NEON_OPT 2
#  else
#     define PNG_ARM_NEON_OPT 0
#  endif
#endif

#if PNG_ARM_NEON_OPT > 0
   


#  define PNG_FILTER_OPTIMIZATIONS png_init_filter_functions_neon

   













#  ifndef PNG_ARM_NEON_IMPLEMENTATION
#     if defined(__ARM_NEON__) || defined(__ARM_NEON)
#        if defined(__clang__)
            




#        elif defined(__GNUC__)
            


#           if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
#              define PNG_ARM_NEON_IMPLEMENTATION 2
#           endif 
#        endif 
#     else 
         

#        define PNG_ARM_NEON_IMPLEMENTATION 2
#     endif 
#  endif 

#  ifndef PNG_ARM_NEON_IMPLEMENTATION
      
#     define PNG_ARM_NEON_IMPLEMENTATION 1
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






#ifndef PNG_INTERNAL_DATA
#  define PNG_INTERNAL_DATA(type, name, array) extern type name array
#endif

#ifndef PNG_INTERNAL_FUNCTION
#  define PNG_INTERNAL_FUNCTION(type, name, args, attributes)\
      extern PNG_FUNCTION(type, name, args, PNG_EMPTY attributes)
#endif

#ifndef PNG_INTERNAL_CALLBACK
#  define PNG_INTERNAL_CALLBACK(type, name, args, attributes)\
      extern PNG_FUNCTION(type, (PNGCBAPI name), args, PNG_EMPTY attributes)
#endif










#ifndef PNG_FP_EXPORT
#  ifndef PNG_FLOATING_POINT_SUPPORTED
#     define PNG_FP_EXPORT(ordinal, type, name, args)\
         PNG_INTERNAL_FUNCTION(type, name, args, PNG_EMPTY);
#     ifndef PNG_VERSION_INFO_ONLY
         typedef struct png_incomplete png_double;
         typedef png_double*           png_doublep;
         typedef const png_double*     png_const_doublep;
         typedef png_double**          png_doublepp;
#     endif
#  endif
#endif
#ifndef PNG_FIXED_EXPORT
#  ifndef PNG_FIXED_POINT_SUPPORTED
#     define PNG_FIXED_EXPORT(ordinal, type, name, args)\
         PNG_INTERNAL_FUNCTION(type, name, args, PNG_EMPTY);
#  endif
#endif

#include "png.h"


#ifndef PNG_DLL_EXPORT
#  define PNG_DLL_EXPORT
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





#ifdef PNG_WARNINGS_SUPPORTED
#  define PNG_WARNING_PARAMETERS(p) png_warning_parameters p;
#else
#  define png_warning_parameter(p,number,string) ((void)0)
#  define png_warning_parameter_unsigned(p,number,format,value) ((void)0)
#  define png_warning_parameter_signed(p,number,format,value) ((void)0)
#  define png_formatted_warning(pp,p,message) ((void)(pp))
#  define PNG_WARNING_PARAMETERS(p)
#endif
#ifndef PNG_ERROR_TEXT_SUPPORTED
#  define png_fixed_error(s1,s2) png_err(s1)
#endif






#ifdef __cplusplus
#  define png_voidcast(type, value) static_cast<type>(value)
#  define png_constcast(type, value) const_cast<type>(value)
#  define png_aligncast(type, value) \
   static_cast<type>(static_cast<void*>(value))
#  define png_aligncastconst(type, value) \
   static_cast<type>(static_cast<const void*>(value))
#else
#  define png_voidcast(type, value) (value)
#  define png_constcast(type, value) ((type)(value))
#  define png_aligncast(type, value) ((void*)(value))
#  define png_aligncastconst(type, value) ((const void*)(value))
#endif 





#ifdef PNG_FIXED_POINT_SUPPORTED
#  define PNGFAPI PNGAPI
#else
#  define PNGFAPI
#endif

#ifndef PNG_VERSION_INFO_ONLY



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
#endif 







#ifndef PNG_ABORT
#  ifdef _WINDOWS_
#    define PNG_ABORT() ExitProcess(0)
#  else
#    define PNG_ABORT() abort()
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
                   
                   
                   
#define PNG_HAVE_CHUNK_HEADER      0x100
#define PNG_WROTE_tIME             0x200
#define PNG_WROTE_INFO_BEFORE_PLTE 0x400
#define PNG_BACKGROUND_IS_GRAY     0x800
#define PNG_HAVE_PNG_SIGNATURE    0x1000
#define PNG_HAVE_CHUNK_AFTER_IDAT 0x2000 /* Have another chunk after IDAT */
                   
#define PNG_IS_READ_STRUCT        0x8000 /* Else is a write struct */
#ifdef PNG_APNG_SUPPORTED
#define PNG_HAVE_acTL            0x10000
#define PNG_HAVE_fcTL            0x20000
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
#define PNG_ADD_ALPHA        0x1000000 /* Added to libpng-1.2.7 */
#define PNG_EXPAND_tRNS      0x2000000 /* Added to libpng-1.2.9 */
#define PNG_SCALE_16_TO_8    0x4000000 /* Added to libpng-1.5.4 */
                       
                       
                       
                       

#define PNG_STRUCT_PNG   0x0001
#define PNG_STRUCT_INFO  0x0002


#define PNG_WEIGHT_FACTOR (1<<(PNG_WEIGHT_SHIFT))
#define PNG_COST_FACTOR (1<<(PNG_COST_SHIFT))


#define PNG_FLAG_ZLIB_CUSTOM_STRATEGY     0x0001
#define PNG_FLAG_ZSTREAM_INITIALIZED      0x0002 /* Added to libpng-1.6.0 */
                                  
#define PNG_FLAG_ZSTREAM_ENDED            0x0008 /* Added to libpng-1.6.0 */
                                  
                                  
#define PNG_FLAG_ROW_INIT                 0x0040
#define PNG_FLAG_FILLER_AFTER             0x0080
#define PNG_FLAG_CRC_ANCILLARY_USE        0x0100
#define PNG_FLAG_CRC_ANCILLARY_NOWARN     0x0200
#define PNG_FLAG_CRC_CRITICAL_USE         0x0400
#define PNG_FLAG_CRC_CRITICAL_IGNORE      0x0800
#define PNG_FLAG_ASSUME_sRGB              0x1000 /* Added to libpng-1.5.4 */
#define PNG_FLAG_OPTIMIZE_ALPHA           0x2000 /* Added to libpng-1.5.4 */
#define PNG_FLAG_DETECT_UNINITIALIZED     0x4000 /* Added to libpng-1.5.4 */


#define PNG_FLAG_LIBRARY_MISMATCH        0x20000
#define PNG_FLAG_STRIP_ERROR_NUMBERS     0x40000
#define PNG_FLAG_STRIP_ERROR_TEXT        0x80000
#define PNG_FLAG_BENIGN_ERRORS_WARN     0x100000 /* Added to libpng-1.4.0 */
#define PNG_FLAG_APP_WARNINGS_WARN      0x200000 /* Added to libpng-1.6.0 */
#define PNG_FLAG_APP_ERRORS_WARN        0x400000 /* Added to libpng-1.6.0 */
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  

#define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)

#define PNG_FLAG_CRC_CRITICAL_MASK  (PNG_FLAG_CRC_CRITICAL_USE | \
                                     PNG_FLAG_CRC_CRITICAL_IGNORE)

#define PNG_FLAG_CRC_MASK           (PNG_FLAG_CRC_ANCILLARY_MASK | \
                                     PNG_FLAG_CRC_CRITICAL_MASK)



#define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))









#define PNG_DIV65535(v24) (((v24) + 32895) >> 16)
#define PNG_DIV257(v16) PNG_DIV65535((png_uint_32)(v16) * 255)


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
#endif



#endif














#define PNG_32b(b,s) ((png_uint_32)(b) << (s))
#define PNG_U32(b1,b2,b3,b4) \
   (PNG_32b(b1,24) | PNG_32b(b2,16) | PNG_32b(b3,8) | PNG_32b(b4,0))
























#define png_IDAT PNG_U32( 73,  68,  65,  84)
#define png_IEND PNG_U32( 73,  69,  78,  68)
#define png_IHDR PNG_U32( 73,  72,  68,  82)
#define png_PLTE PNG_U32( 80,  76,  84,  69)
#define png_bKGD PNG_U32( 98,  75,  71,  68)
#define png_cHRM PNG_U32( 99,  72,  82,  77)
#define png_fRAc PNG_U32(102,  82,  65,  99) /* registered, not defined */
#define png_gAMA PNG_U32(103,  65,  77,  65)
#define png_gIFg PNG_U32(103,  73,  70, 103)
#define png_gIFt PNG_U32(103,  73,  70, 116) /* deprecated */
#define png_gIFx PNG_U32(103,  73,  70, 120)
#define png_hIST PNG_U32(104,  73,  83,  84)
#define png_iCCP PNG_U32(105,  67,  67,  80)
#define png_iTXt PNG_U32(105,  84,  88, 116)
#define png_oFFs PNG_U32(111,  70,  70, 115)
#define png_pCAL PNG_U32(112,  67,  65,  76)
#define png_pHYs PNG_U32(112,  72,  89, 115)
#define png_sBIT PNG_U32(115,  66,  73,  84)
#define png_sCAL PNG_U32(115,  67,  65,  76)
#define png_sPLT PNG_U32(115,  80,  76,  84)
#define png_sRGB PNG_U32(115,  82,  71,  66)
#define png_sTER PNG_U32(115,  84,  69,  82)
#define png_tEXt PNG_U32(116,  69,  88, 116)
#define png_tIME PNG_U32(116,  73,  77,  69)
#define png_tRNS PNG_U32(116,  82,  78,  83)
#define png_zTXt PNG_U32(122,  84,  88, 116)

#ifdef PNG_APNG_SUPPORTED
#define png_acTL PNG_U32( 97,  99,  84,  76)
#define png_fcTL PNG_U32(102,  99,  84,  76)
#define png_fdAT PNG_U32(102, 100,  65,  84)


#define PNG_FIRST_FRAME_HIDDEN       0x0001
#define PNG_APNG_APP                 0x0002
#endif




#define PNG_CHUNK_FROM_STRING(s)\
   PNG_U32(0xff & (s)[0], 0xff & (s)[1], 0xff & (s)[2], 0xff & (s)[3])





#define PNG_STRING_FROM_CHUNK(s,c)\
   (void)(((char*)(s))[0]=(char)(((c)>>24) & 0xff), \
   ((char*)(s))[1]=(char)(((c)>>16) & 0xff),\
   ((char*)(s))[2]=(char)(((c)>>8) & 0xff), \
   ((char*)(s))[3]=(char)((c & 0xff)))


#define PNG_CSTRING_FROM_CHUNK(s,c)\
   (void)(PNG_STRING_FROM_CHUNK(s,c), ((char*)(s))[4] = 0)


#define PNG_CHUNK_ANCILLARY(c)   (1 & ((c) >> 29))
#define PNG_CHUNK_CRITICAL(c)     (!PNG_CHUNK_ANCILLARY(c))
#define PNG_CHUNK_PRIVATE(c)      (1 & ((c) >> 21))
#define PNG_CHUNK_RESERVED(c)     (1 & ((c) >> 13))
#define PNG_CHUNK_SAFE_TO_COPY(c) (1 & ((c) >>  5))


#define PNG_GAMMA_MAC_OLD 151724  /* Assume '1.8' is really 2.2/1.45! */
#define PNG_GAMMA_MAC_INVERSE 65909
#define PNG_GAMMA_sRGB_INVERSE 45455




#ifndef PNG_VERSION_INFO_ONLY

#include "pngstruct.h"
#include "pnginfo.h"




#if PNG_ZLIB_VERNUM != 0 && PNG_ZLIB_VERNUM != ZLIB_VERNUM
#  error ZLIB_VERNUM != PNG_ZLIB_VERNUM \
      "-I (include path) error: see the notes in pngpriv.h"
   









#endif




typedef const png_uint_16p * png_const_uint_16pp;


#if defined(PNG_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(PNG_SIMPLIFIED_WRITE_SUPPORTED)
#ifdef PNG_SIMPLIFIED_READ_SUPPORTED
PNG_INTERNAL_DATA(const png_uint_16, png_sRGB_table, [256]);
   


#endif

PNG_INTERNAL_DATA(const png_uint_16, png_sRGB_base, [512]);
PNG_INTERNAL_DATA(const png_byte, png_sRGB_delta, [512]);

#define PNG_sRGB_FROM_LINEAR(linear) \
  ((png_byte)(0xff & ((png_sRGB_base[(linear)>>15] \
   + ((((linear) & 0x7fff)*png_sRGB_delta[(linear)>>15])>>12)) >> 8)))
   


#endif 



#ifdef __cplusplus
extern "C" {
#endif








#define PNG_UNEXPECTED_ZLIB_RETURN (-7)
PNG_INTERNAL_FUNCTION(void, png_zstream_error,(png_structrp png_ptr, int ret),
   PNG_EMPTY);
   



#ifdef PNG_WRITE_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_free_buffer_list,(png_structrp png_ptr,
   png_compression_bufferp *list),PNG_EMPTY);
   
#endif

#if defined(PNG_FLOATING_POINT_SUPPORTED) && \
   !defined(PNG_FIXED_POINT_MACRO_SUPPORTED) && \
   (defined(PNG_gAMA_SUPPORTED) || defined(PNG_cHRM_SUPPORTED) || \
   defined(PNG_sCAL_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)) || \
   (defined(PNG_sCAL_SUPPORTED) && \
   defined(PNG_FLOATING_ARITHMETIC_SUPPORTED))
PNG_INTERNAL_FUNCTION(png_fixed_point,png_fixed,(png_const_structrp png_ptr,
   double fp, png_const_charp text),PNG_EMPTY);
#endif




PNG_INTERNAL_FUNCTION(int,png_user_version_check,(png_structrp png_ptr,
   png_const_charp user_png_ver),PNG_EMPTY);





PNG_INTERNAL_FUNCTION(png_voidp,png_malloc_base,(png_const_structrp png_ptr,
   png_alloc_size_t size),PNG_ALLOCATED);

#if defined(PNG_TEXT_SUPPORTED) || defined(PNG_sPLT_SUPPORTED) ||\
   defined(PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED)



PNG_INTERNAL_FUNCTION(png_voidp,png_malloc_array,(png_const_structrp png_ptr,
   int nelements, size_t element_size),PNG_ALLOCATED);





PNG_INTERNAL_FUNCTION(png_voidp,png_realloc_array,(png_const_structrp png_ptr,
   png_const_voidp array, int old_elements, int add_elements,
   size_t element_size),PNG_ALLOCATED);
#endif 







PNG_INTERNAL_FUNCTION(png_structp,png_create_png_struct,
   (png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn,
    png_error_ptr warn_fn, png_voidp mem_ptr, png_malloc_ptr malloc_fn,
    png_free_ptr free_fn),PNG_ALLOCATED);


PNG_INTERNAL_FUNCTION(void,png_destroy_png_struct,(png_structrp png_ptr),
   PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_free_jmpbuf,(png_structrp png_ptr),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(voidpf,png_zalloc,(voidpf png_ptr, uInt items, uInt size),
   PNG_ALLOCATED);


PNG_INTERNAL_FUNCTION(void,png_zfree,(voidpf png_ptr, voidpf ptr),PNG_EMPTY);






PNG_INTERNAL_FUNCTION(void PNGCBAPI,png_default_read_data,(png_structp png_ptr,
    png_bytep data, png_size_t length),PNG_EMPTY);

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_INTERNAL_FUNCTION(void PNGCBAPI,png_push_fill_buffer,(png_structp png_ptr,
    png_bytep buffer, png_size_t length),PNG_EMPTY);
#endif

PNG_INTERNAL_FUNCTION(void PNGCBAPI,png_default_write_data,(png_structp png_ptr,
    png_bytep data, png_size_t length),PNG_EMPTY);

#ifdef PNG_WRITE_FLUSH_SUPPORTED
#  ifdef PNG_STDIO_SUPPORTED
PNG_INTERNAL_FUNCTION(void PNGCBAPI,png_default_flush,(png_structp png_ptr),
   PNG_EMPTY);
#  endif
#endif


PNG_INTERNAL_FUNCTION(void,png_reset_crc,(png_structrp png_ptr),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_write_data,(png_structrp png_ptr,
    png_const_bytep data, png_size_t length),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_read_sig,(png_structrp png_ptr,
   png_inforp info_ptr),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(png_uint_32,png_read_chunk_header,(png_structrp png_ptr),
   PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_read_data,(png_structrp png_ptr, png_bytep data,
    png_size_t length),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_crc_read,(png_structrp png_ptr, png_bytep buf,
    png_uint_32 length),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(int,png_crc_finish,(png_structrp png_ptr,
   png_uint_32 skip),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(int,png_crc_error,(png_structrp png_ptr),PNG_EMPTY);





PNG_INTERNAL_FUNCTION(void,png_calculate_crc,(png_structrp png_ptr,
   png_const_bytep ptr, png_size_t length),PNG_EMPTY);

#ifdef PNG_WRITE_FLUSH_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_flush,(png_structrp png_ptr),PNG_EMPTY);
#endif






PNG_INTERNAL_FUNCTION(void,png_write_IHDR,(png_structrp png_ptr,
   png_uint_32 width, png_uint_32 height, int bit_depth, int color_type,
   int compression_method, int filter_method, int interlace_method),PNG_EMPTY);

PNG_INTERNAL_FUNCTION(void,png_write_PLTE,(png_structrp png_ptr,
   png_const_colorp palette, png_uint_32 num_pal),PNG_EMPTY);

PNG_INTERNAL_FUNCTION(void,png_compress_IDAT,(png_structrp png_ptr,
   png_const_bytep row_data, png_alloc_size_t row_data_length, int flush),
   PNG_EMPTY);

PNG_INTERNAL_FUNCTION(void,png_write_IEND,(png_structrp png_ptr),PNG_EMPTY);

#ifdef PNG_WRITE_gAMA_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_gAMA_fixed,(png_structrp png_ptr,
    png_fixed_point file_gamma),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_sBIT_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_sBIT,(png_structrp png_ptr,
    png_const_color_8p sbit, int color_type),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_cHRM_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_cHRM_fixed,(png_structrp png_ptr,
    const png_xy *xy), PNG_EMPTY);
    
#endif

#ifdef PNG_WRITE_sRGB_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_sRGB,(png_structrp png_ptr,
    int intent),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_iCCP_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_iCCP,(png_structrp png_ptr,
   png_const_charp name, png_const_bytep profile), PNG_EMPTY);
   



#endif

#ifdef PNG_WRITE_sPLT_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_sPLT,(png_structrp png_ptr,
    png_const_sPLT_tp palette),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_tRNS_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_tRNS,(png_structrp png_ptr,
    png_const_bytep trans, png_const_color_16p values, int number,
    int color_type),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_bKGD_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_bKGD,(png_structrp png_ptr,
    png_const_color_16p values, int color_type),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_hIST,(png_structrp png_ptr,
    png_const_uint_16p hist, int num_hist),PNG_EMPTY);
#endif


#ifdef PNG_WRITE_tEXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_tEXt,(png_structrp png_ptr,
   png_const_charp key, png_const_charp text, png_size_t text_len),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_zTXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_zTXt,(png_structrp png_ptr, png_const_charp
    key, png_const_charp text, int compression),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_iTXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_iTXt,(png_structrp png_ptr,
    int compression, png_const_charp key, png_const_charp lang,
    png_const_charp lang_key, png_const_charp text),PNG_EMPTY);
#endif

#ifdef PNG_TEXT_SUPPORTED  
PNG_INTERNAL_FUNCTION(int,png_set_text_2,(png_const_structrp png_ptr,
    png_inforp info_ptr, png_const_textp text_ptr, int num_text),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_oFFs,(png_structrp png_ptr,
    png_int_32 x_offset, png_int_32 y_offset, int unit_type),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_pCAL_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_pCAL,(png_structrp png_ptr,
    png_charp purpose, png_int_32 X0, png_int_32 X1, int type, int nparams,
    png_const_charp units, png_charpp params),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_pHYs_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_pHYs,(png_structrp png_ptr,
    png_uint_32 x_pixels_per_unit, png_uint_32 y_pixels_per_unit,
    int unit_type),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_tIME_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_tIME,(png_structrp png_ptr,
    png_const_timep mod_time),PNG_EMPTY);
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_sCAL_s,(png_structrp png_ptr,
    int unit, png_const_charp width, png_const_charp height),PNG_EMPTY);
#endif


PNG_INTERNAL_FUNCTION(void,png_write_finish_row,(png_structrp png_ptr),
    PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_write_start_row,(png_structrp png_ptr),
    PNG_EMPTY);
























#ifndef PNG_USE_COMPILE_TIME_MASKS
#  define PNG_USE_COMPILE_TIME_MASKS 1
#endif
PNG_INTERNAL_FUNCTION(void,png_combine_row,(png_const_structrp png_ptr,
    png_bytep row, int display),PNG_EMPTY);

#ifdef PNG_READ_INTERLACING_SUPPORTED






PNG_INTERNAL_FUNCTION(void,png_do_read_interlace,(png_row_infop row_info,
    png_bytep row, int pass, png_uint_32 transformations),PNG_EMPTY);
#endif



#ifdef PNG_WRITE_INTERLACING_SUPPORTED

PNG_INTERNAL_FUNCTION(void,png_do_write_interlace,(png_row_infop row_info,
    png_bytep row, int pass),PNG_EMPTY);
#endif




PNG_INTERNAL_FUNCTION(void,png_read_filter_row,(png_structrp pp, png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row, int filter),PNG_EMPTY);

PNG_INTERNAL_FUNCTION(void,png_read_filter_row_up_neon,(png_row_infop row_info,
    png_bytep row, png_const_bytep prev_row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_filter_row_sub3_neon,(png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_filter_row_sub4_neon,(png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_filter_row_avg3_neon,(png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_filter_row_avg4_neon,(png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_filter_row_paeth3_neon,(png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_filter_row_paeth4_neon,(png_row_infop
    row_info, png_bytep row, png_const_bytep prev_row),PNG_EMPTY);


PNG_INTERNAL_FUNCTION(void,png_write_find_filter,(png_structrp png_ptr,
    png_row_infop row_info),PNG_EMPTY);

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_read_IDAT_data,(png_structrp png_ptr,
   png_bytep output, png_alloc_size_t avail_out),PNG_EMPTY);
   




PNG_INTERNAL_FUNCTION(void,png_read_finish_IDAT,(png_structrp png_ptr),
   PNG_EMPTY);
   



PNG_INTERNAL_FUNCTION(void,png_read_finish_row,(png_structrp png_ptr),
   PNG_EMPTY);
   
#endif 


PNG_INTERNAL_FUNCTION(void,png_read_start_row,(png_structrp png_ptr),PNG_EMPTY);

#ifdef PNG_READ_TRANSFORMS_SUPPORTED

PNG_INTERNAL_FUNCTION(void,png_read_transform_info,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
#endif


#if defined(PNG_WRITE_FILLER_SUPPORTED) || \
    defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_do_strip_channel,(png_row_infop row_info,
    png_bytep row, int at_start),PNG_EMPTY);
#endif

#ifdef PNG_16BIT_SUPPORTED
#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_do_swap,(png_row_infop row_info,
    png_bytep row),PNG_EMPTY);
#endif
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || \
    defined(PNG_WRITE_PACKSWAP_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_do_packswap,(png_row_infop row_info,
    png_bytep row),PNG_EMPTY);
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_do_invert,(png_row_infop row_info,
    png_bytep row),PNG_EMPTY);
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_do_bgr,(png_row_infop row_info,
    png_bytep row),PNG_EMPTY);
#endif






PNG_INTERNAL_FUNCTION(void,png_handle_IHDR,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_handle_PLTE,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_handle_IEND,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);

#ifdef PNG_READ_bKGD_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_bKGD,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_cHRM_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_cHRM,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_gAMA_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_gAMA,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_hIST_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_hIST,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_iCCP_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_iCCP,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif 

#ifdef PNG_READ_iTXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_iTXt,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_oFFs,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_pCAL,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_pHYs,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_sBIT,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_sCAL_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_sCAL,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_sPLT_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_sPLT,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif 

#ifdef PNG_READ_sRGB_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_sRGB,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_tEXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_tEXt,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_tIME_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_tIME,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_tRNS_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_tRNS,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

#ifdef PNG_READ_zTXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_zTXt,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
#endif

PNG_INTERNAL_FUNCTION(void,png_check_chunk_name,(png_structrp png_ptr,
    png_uint_32 chunk_name),PNG_EMPTY);

PNG_INTERNAL_FUNCTION(void,png_handle_unknown,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length, int keep),PNG_EMPTY);
   





#if defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED) ||\
    defined(PNG_HANDLE_AS_UNKNOWN_SUPPORTED)
PNG_INTERNAL_FUNCTION(int,png_chunk_unknown_handling,
    (png_const_structrp png_ptr, png_uint_32 chunk_name),PNG_EMPTY);
   


#endif 


#ifdef PNG_READ_TRANSFORMS_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_do_read_transformations,(png_structrp png_ptr,
   png_row_infop row_info),PNG_EMPTY);
#endif
#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_do_write_transformations,(png_structrp png_ptr,
   png_row_infop row_info),PNG_EMPTY);
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_init_read_transformations,(png_structrp png_ptr),
    PNG_EMPTY);
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_push_read_chunk,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_read_sig,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_check_crc,(png_structrp png_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_crc_skip,(png_structrp png_ptr,
    png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_crc_finish,(png_structrp png_ptr),
    PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_save_buffer,(png_structrp png_ptr),
    PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_restore_buffer,(png_structrp png_ptr,
    png_bytep buffer, png_size_t buffer_length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_read_IDAT,(png_structrp png_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_process_IDAT_data,(png_structrp png_ptr,
    png_bytep buffer, png_size_t buffer_length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_process_row,(png_structrp png_ptr),
    PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_handle_unknown,(png_structrp png_ptr,
   png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_have_info,(png_structrp png_ptr,
   png_inforp info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_have_end,(png_structrp png_ptr,
   png_inforp info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_have_row,(png_structrp png_ptr,
     png_bytep row),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_read_end,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_process_some_data,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_push_finish_row,(png_structrp png_ptr),
    PNG_EMPTY);
#  ifdef PNG_READ_tEXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_push_handle_tEXt,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_read_tEXt,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
#  endif
#  ifdef PNG_READ_zTXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_push_handle_zTXt,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_read_zTXt,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
#  endif
#  ifdef PNG_READ_iTXt_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_push_handle_iTXt,(png_structrp png_ptr,
    png_inforp info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_push_read_iTXt,(png_structrp png_ptr,
    png_inforp info_ptr),PNG_EMPTY);
#  endif

#endif 

#ifdef PNG_APNG_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_ensure_fcTL_is_valid,(png_structp png_ptr,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den,
   png_byte dispose_op, png_byte blend_op),PNG_EMPTY);

#ifdef PNG_READ_APNG_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_handle_acTL,(png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_handle_fcTL,(png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_handle_fdAT,(png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_have_info,(png_structp png_ptr,
   png_infop info_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_ensure_sequence_number,(png_structp png_ptr,
   png_uint_32 length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_reset,(png_structp png_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_read_reinit,(png_structp png_ptr,
   png_infop info_ptr),PNG_EMPTY);
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_progressive_read_reset,(png_structp png_ptr),
   PNG_EMPTY);
#endif 
#endif 

#ifdef PNG_WRITE_APNG_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_write_acTL,(png_structp png_ptr,
   png_uint_32 num_frames, png_uint_32 num_plays),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_write_fcTL,(png_structp png_ptr,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den,
   png_byte dispose_op, png_byte blend_op),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_write_fdAT,(png_structp png_ptr,
   png_const_bytep data, png_size_t length),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_write_reset,(png_structp png_ptr),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_write_reinit,(png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height),PNG_EMPTY);
#endif 
#endif 


#ifdef PNG_GAMMA_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_colorspace_set_gamma,(png_const_structrp png_ptr,
    png_colorspacerp colorspace, png_fixed_point gAMA), PNG_EMPTY);
   




PNG_INTERNAL_FUNCTION(void,png_colorspace_sync_info,(png_const_structrp png_ptr,
    png_inforp info_ptr), PNG_EMPTY);
    

PNG_INTERNAL_FUNCTION(void,png_colorspace_sync,(png_const_structrp png_ptr,
    png_inforp info_ptr), PNG_EMPTY);
    


#endif


#ifdef PNG_COLORSPACE_SUPPORTED



PNG_INTERNAL_FUNCTION(int,png_colorspace_set_chromaticities,
   (png_const_structrp png_ptr, png_colorspacerp colorspace, const png_xy *xy,
    int preferred), PNG_EMPTY);

PNG_INTERNAL_FUNCTION(int,png_colorspace_set_endpoints,
   (png_const_structrp png_ptr, png_colorspacerp colorspace, const png_XYZ *XYZ,
    int preferred), PNG_EMPTY);

#ifdef PNG_sRGB_SUPPORTED
PNG_INTERNAL_FUNCTION(int,png_colorspace_set_sRGB,(png_const_structrp png_ptr,
   png_colorspacerp colorspace, int intent), PNG_EMPTY);
   




#endif 

#ifdef PNG_iCCP_SUPPORTED
PNG_INTERNAL_FUNCTION(int,png_colorspace_set_ICC,(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_const_charp name,
   png_uint_32 profile_length, png_const_bytep profile, int color_type),
   PNG_EMPTY);
   


PNG_INTERNAL_FUNCTION(int,png_icc_check_length,(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_const_charp name,
   png_uint_32 profile_length), PNG_EMPTY);
PNG_INTERNAL_FUNCTION(int,png_icc_check_header,(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_const_charp name,
   png_uint_32 profile_length,
   png_const_bytep profile , int color_type),
   PNG_EMPTY);
PNG_INTERNAL_FUNCTION(int,png_icc_check_tag_table,(png_const_structrp png_ptr,
   png_colorspacerp colorspace, png_const_charp name,
   png_uint_32 profile_length,
   png_const_bytep profile ), PNG_EMPTY);
#ifdef PNG_sRGB_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_icc_set_sRGB,(
   png_const_structrp png_ptr, png_colorspacerp colorspace,
   png_const_bytep profile, uLong adler), PNG_EMPTY);
   



#endif
#endif 

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_colorspace_set_rgb_coefficients,
   (png_structrp png_ptr), PNG_EMPTY);
   
#endif 
#endif 


PNG_INTERNAL_FUNCTION(void,png_check_IHDR,(png_const_structrp png_ptr,
    png_uint_32 width, png_uint_32 height, int bit_depth,
    int color_type, int interlace_type, int compression_type,
    int filter_type),PNG_EMPTY);


#if defined(PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED) || \
    defined(PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_do_check_palette_indexes,
   (png_structrp png_ptr, png_row_infop row_info),PNG_EMPTY);
#endif

#if defined(PNG_FLOATING_POINT_SUPPORTED) && defined(PNG_ERROR_TEXT_SUPPORTED)
PNG_INTERNAL_FUNCTION(void,png_fixed_error,(png_const_structrp png_ptr,
   png_const_charp name),PNG_NORETURN);
#endif





PNG_INTERNAL_FUNCTION(size_t,png_safecat,(png_charp buffer, size_t bufsize,
   size_t pos, png_const_charp string),PNG_EMPTY);




#if defined(PNG_WARNINGS_SUPPORTED) || defined(PNG_TIME_RFC1123_SUPPORTED)





PNG_INTERNAL_FUNCTION(png_charp,png_format_number,(png_const_charp start,
   png_charp end, int format, png_alloc_size_t number),PNG_EMPTY);


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
#  define PNG_WARNING_PARAMETER_COUNT 8 /* Maximum 9; see pngerror.c */




typedef char png_warning_parameters[PNG_WARNING_PARAMETER_COUNT][
   PNG_WARNING_PARAMETER_SIZE];

PNG_INTERNAL_FUNCTION(void,png_warning_parameter,(png_warning_parameters p,
   int number, png_const_charp string),PNG_EMPTY);
   


PNG_INTERNAL_FUNCTION(void,png_warning_parameter_unsigned,
   (png_warning_parameters p, int number, int format, png_alloc_size_t value),
   PNG_EMPTY);
   


PNG_INTERNAL_FUNCTION(void,png_warning_parameter_signed,
   (png_warning_parameters p, int number, int format, png_int_32 value),
   PNG_EMPTY);

PNG_INTERNAL_FUNCTION(void,png_formatted_warning,(png_const_structrp png_ptr,
   png_warning_parameters p, png_const_charp message),PNG_EMPTY);
   



#endif

#ifdef PNG_BENIGN_ERRORS_SUPPORTED














PNG_INTERNAL_FUNCTION(void,png_app_warning,(png_const_structrp png_ptr,
   png_const_charp message),PNG_EMPTY);
   



PNG_INTERNAL_FUNCTION(void,png_app_error,(png_const_structrp png_ptr,
   png_const_charp message),PNG_EMPTY);
   


#else
#  define png_app_warning(pp,s) png_warning(pp,s)
#  define png_app_error(pp,s) png_error(pp,s)
#endif

PNG_INTERNAL_FUNCTION(void,png_chunk_report,(png_const_structrp png_ptr,
   png_const_charp message, int error),PNG_EMPTY);
   








#define PNG_CHUNK_WARNING     0 /* never an error */
#define PNG_CHUNK_WRITE_ERROR 1 /* an error only on write */
#define PNG_CHUNK_ERROR       2 /* always an error */




#if defined(PNG_sCAL_SUPPORTED)





#define PNG_sCAL_MAX_DIGITS (PNG_sCAL_PRECISION+1/*.*/+1/*E*/+10/*exponent*/)

#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_ascii_from_fp,(png_const_structrp png_ptr,
   png_charp ascii, png_size_t size, double fp, unsigned int precision),
   PNG_EMPTY);
#endif 

#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_INTERNAL_FUNCTION(void,png_ascii_from_fixed,(png_const_structrp png_ptr,
   png_charp ascii, png_size_t size, png_fixed_point fp),PNG_EMPTY);
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
















PNG_INTERNAL_FUNCTION(int,png_check_fp_number,(png_const_charp string,
   png_size_t size, int *statep, png_size_tp whereami),PNG_EMPTY);







PNG_INTERNAL_FUNCTION(int,png_check_fp_string,(png_const_charp string,
   png_size_t size),PNG_EMPTY);
#endif 

#if defined(PNG_GAMMA_SUPPORTED) ||\
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG_READ_pHYs_SUPPORTED)






PNG_INTERNAL_FUNCTION(int,png_muldiv,(png_fixed_point_p res, png_fixed_point a,
   png_int_32 multiplied_by, png_int_32 divided_by),PNG_EMPTY);
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_INCH_CONVERSIONS_SUPPORTED)

PNG_INTERNAL_FUNCTION(png_fixed_point,png_muldiv_warn,
   (png_const_structrp png_ptr, png_fixed_point a, png_int_32 multiplied_by,
   png_int_32 divided_by),PNG_EMPTY);
#endif

#ifdef PNG_GAMMA_SUPPORTED




PNG_INTERNAL_FUNCTION(png_fixed_point,png_reciprocal,(png_fixed_point a),
   PNG_EMPTY);

#ifdef PNG_READ_GAMMA_SUPPORTED




PNG_INTERNAL_FUNCTION(png_fixed_point,png_reciprocal2,(png_fixed_point a,
   png_fixed_point b),PNG_EMPTY);
#endif


PNG_INTERNAL_FUNCTION(int,png_gamma_significant,(png_fixed_point gamma_value),
   PNG_EMPTY);
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED







PNG_INTERNAL_FUNCTION(png_uint_16,png_gamma_correct,(png_structrp png_ptr,
   unsigned int value, png_fixed_point gamma_value),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(png_uint_16,png_gamma_16bit_correct,(unsigned int value,
   png_fixed_point gamma_value),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(png_byte,png_gamma_8bit_correct,(unsigned int value,
   png_fixed_point gamma_value),PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_destroy_gamma_table,(png_structrp png_ptr),
   PNG_EMPTY);
PNG_INTERNAL_FUNCTION(void,png_build_gamma_table,(png_structrp png_ptr,
   int bit_depth),PNG_EMPTY);
#endif


#if defined(PNG_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(PNG_SIMPLIFIED_WRITE_SUPPORTED)

typedef struct png_control
{
   png_structp png_ptr;
   png_infop   info_ptr;
   png_voidp   error_buf;           

   png_const_bytep memory;          
   png_size_t      size;            

   unsigned int for_write       :1; 
   unsigned int owned_file      :1; 
} png_control;




#ifdef __cplusplus
#  define png_control_jmp_buf(pc) (((jmp_buf*)((pc)->error_buf))[0])
#else
#  define png_control_jmp_buf(pc) ((pc)->error_buf)
#endif





PNG_INTERNAL_CALLBACK(void,png_safe_error,(png_structp png_ptr,
   png_const_charp error_message),PNG_NORETURN);

#ifdef PNG_WARNINGS_SUPPORTED
PNG_INTERNAL_CALLBACK(void,png_safe_warning,(png_structp png_ptr,
   png_const_charp warning_message),PNG_EMPTY);
#else
#  define png_safe_warning 0/*dummy argument*/
#endif

PNG_INTERNAL_FUNCTION(int,png_safe_execute,(png_imagep image,
   int (*function)(png_voidp), png_voidp arg),PNG_EMPTY);




PNG_INTERNAL_FUNCTION(int,png_image_error,(png_imagep image,
   png_const_charp error_message),PNG_EMPTY);

#ifndef PNG_SIMPLIFIED_READ_SUPPORTED

PNG_INTERNAL_FUNCTION(void, png_image_free, (png_imagep image), PNG_EMPTY);
#endif 

#endif 






#ifdef PNG_FILTER_OPTIMIZATIONS
PNG_INTERNAL_FUNCTION(void, PNG_FILTER_OPTIMIZATIONS, (png_structp png_ptr,
   unsigned int bpp), PNG_EMPTY);
   
#else
   



PNG_INTERNAL_FUNCTION(void, png_init_filter_functions_neon,
   (png_structp png_ptr, unsigned int bpp), PNG_EMPTY);
#endif



#include "pngdebug.h"

#ifdef __cplusplus
}
#endif

#endif
#endif
