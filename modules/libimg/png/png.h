

























































































































































































































































































































































































































































#ifndef PNG_H
#define PNG_H








#define PNG_LIBPNG_VER_STRING "1.2.40"
#define PNG_HEADER_VERSION_STRING \
   " libpng version 1.2.40 - September 10, 2009\n"

#define PNG_LIBPNG_VER_SONUM   0
#define PNG_LIBPNG_VER_DLLNUM  13


#define PNG_LIBPNG_VER_MAJOR   1
#define PNG_LIBPNG_VER_MINOR   2
#define PNG_LIBPNG_VER_RELEASE 40




#define PNG_LIBPNG_VER_BUILD  0


#define PNG_LIBPNG_BUILD_ALPHA    1
#define PNG_LIBPNG_BUILD_BETA     2
#define PNG_LIBPNG_BUILD_RC       3
#define PNG_LIBPNG_BUILD_STABLE   4
#define PNG_LIBPNG_BUILD_RELEASE_STATUS_MASK 7


#define PNG_LIBPNG_BUILD_PATCH    8 /* Can be OR'ed with
                                       PNG_LIBPNG_BUILD_STABLE only */
#define PNG_LIBPNG_BUILD_PRIVATE 16 /* Cannot be OR'ed with
                                       PNG_LIBPNG_BUILD_SPECIAL */
#define PNG_LIBPNG_BUILD_SPECIAL 32 /* Cannot be OR'ed with
                                       PNG_LIBPNG_BUILD_PRIVATE */

#define PNG_LIBPNG_BUILD_BASE_TYPE PNG_LIBPNG_BUILD_STABLE







#define PNG_LIBPNG_VER 10240 /* 1.2.40 */

#ifndef PNG_VERSION_INFO_ONLY

#include "zlib.h"
#endif


#include "pngconf.h"














#if defined(PNG_USER_PRIVATEBUILD)
#  define PNG_LIBPNG_BUILD_TYPE \
          (PNG_LIBPNG_BUILD_BASE_TYPE | PNG_LIBPNG_BUILD_PRIVATE)
#else
#  if defined(PNG_LIBPNG_SPECIALBUILD)
#    define PNG_LIBPNG_BUILD_TYPE \
            (PNG_LIBPNG_BUILD_BASE_TYPE | PNG_LIBPNG_BUILD_SPECIAL)
#  else
#    define PNG_LIBPNG_BUILD_TYPE (PNG_LIBPNG_BUILD_BASE_TYPE)
#  endif
#endif

#ifndef PNG_VERSION_INFO_ONLY


#ifdef __cplusplus
extern "C" {
#endif







#ifndef PNG_NO_TYPECAST_NULL
#define int_p_NULL                (int *)NULL
#define png_bytep_NULL            (png_bytep)NULL
#define png_bytepp_NULL           (png_bytepp)NULL
#define png_doublep_NULL          (png_doublep)NULL
#define png_error_ptr_NULL        (png_error_ptr)NULL
#define png_flush_ptr_NULL        (png_flush_ptr)NULL
#define png_free_ptr_NULL         (png_free_ptr)NULL
#define png_infopp_NULL           (png_infopp)NULL
#define png_malloc_ptr_NULL       (png_malloc_ptr)NULL
#define png_read_status_ptr_NULL  (png_read_status_ptr)NULL
#define png_rw_ptr_NULL           (png_rw_ptr)NULL
#define png_structp_NULL          (png_structp)NULL
#define png_uint_16p_NULL         (png_uint_16p)NULL
#define png_voidp_NULL            (png_voidp)NULL
#define png_write_status_ptr_NULL (png_write_status_ptr)NULL
#else
#define int_p_NULL                NULL
#define png_bytep_NULL            NULL
#define png_bytepp_NULL           NULL
#define png_doublep_NULL          NULL
#define png_error_ptr_NULL        NULL
#define png_flush_ptr_NULL        NULL
#define png_free_ptr_NULL         NULL
#define png_infopp_NULL           NULL
#define png_malloc_ptr_NULL       NULL
#define png_read_status_ptr_NULL  NULL
#define png_rw_ptr_NULL           NULL
#define png_structp_NULL          NULL
#define png_uint_16p_NULL         NULL
#define png_voidp_NULL            NULL
#define png_write_status_ptr_NULL NULL
#endif


#if !defined(PNG_NO_EXTERN) || defined(PNG_ALWAYS_EXTERN)



#ifdef PNG_USE_GLOBAL_ARRAYS
PNG_EXPORT_VAR (PNG_CONST char) png_libpng_ver[18];
  
#else
#define png_libpng_ver png_get_header_ver(NULL)
#endif

#ifdef PNG_USE_GLOBAL_ARRAYS


PNG_EXPORT_VAR (PNG_CONST int FARDATA) png_pass_start[7];
PNG_EXPORT_VAR (PNG_CONST int FARDATA) png_pass_inc[7];
PNG_EXPORT_VAR (PNG_CONST int FARDATA) png_pass_ystart[7];
PNG_EXPORT_VAR (PNG_CONST int FARDATA) png_pass_yinc[7];
PNG_EXPORT_VAR (PNG_CONST int FARDATA) png_pass_mask[7];
PNG_EXPORT_VAR (PNG_CONST int FARDATA) png_pass_dsp_mask[7];



#endif

#endif 





typedef struct png_color_struct
{
   png_byte red;
   png_byte green;
   png_byte blue;
} png_color;
typedef png_color FAR * png_colorp;
typedef png_color FAR * FAR * png_colorpp;

typedef struct png_color_16_struct
{
   png_byte index;    
   png_uint_16 red;   
   png_uint_16 green;
   png_uint_16 blue;
   png_uint_16 gray;  
} png_color_16;
typedef png_color_16 FAR * png_color_16p;
typedef png_color_16 FAR * FAR * png_color_16pp;

typedef struct png_color_8_struct
{
   png_byte red;   
   png_byte green;
   png_byte blue;
   png_byte gray;  
   png_byte alpha; 
} png_color_8;
typedef png_color_8 FAR * png_color_8p;
typedef png_color_8 FAR * FAR * png_color_8pp;





typedef struct png_sPLT_entry_struct
{
   png_uint_16 red;
   png_uint_16 green;
   png_uint_16 blue;
   png_uint_16 alpha;
   png_uint_16 frequency;
} png_sPLT_entry;
typedef png_sPLT_entry FAR * png_sPLT_entryp;
typedef png_sPLT_entry FAR * FAR * png_sPLT_entrypp;






typedef struct png_sPLT_struct
{
   png_charp name;           
   png_byte depth;           
   png_sPLT_entryp entries;  
   png_int_32 nentries;      
} png_sPLT_t;
typedef png_sPLT_t FAR * png_sPLT_tp;
typedef png_sPLT_t FAR * FAR * png_sPLT_tpp;

#ifdef PNG_TEXT_SUPPORTED








typedef struct png_text_struct
{
   int  compression;       




   png_charp key;          
   png_charp text;         

   png_size_t text_length; 
#ifdef PNG_iTXt_SUPPORTED
   png_size_t itxt_length; 
   png_charp lang;         

   png_charp lang_key;     

#endif
} png_text;
typedef png_text FAR * png_textp;
typedef png_text FAR * FAR * png_textpp;
#endif




#define PNG_TEXT_COMPRESSION_NONE_WR -3
#define PNG_TEXT_COMPRESSION_zTXt_WR -2
#define PNG_TEXT_COMPRESSION_NONE    -1
#define PNG_TEXT_COMPRESSION_zTXt     0
#define PNG_ITXT_COMPRESSION_NONE     1
#define PNG_ITXT_COMPRESSION_zTXt     2
#define PNG_TEXT_COMPRESSION_LAST     3  /* Not a valid value */







typedef struct png_time_struct
{
   png_uint_16 year; 
   png_byte month;   
   png_byte day;     
   png_byte hour;    
   png_byte minute;  
   png_byte second;  
} png_time;
typedef png_time FAR * png_timep;
typedef png_time FAR * FAR * png_timepp;

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED) || \
 defined(PNG_HANDLE_AS_UNKNOWN_SUPPORTED)





#define PNG_CHUNK_NAME_LENGTH 5
typedef struct png_unknown_chunk_t
{
    png_byte name[PNG_CHUNK_NAME_LENGTH];
    png_byte *data;
    png_size_t size;

    
    png_byte location; 
}
png_unknown_chunk;
typedef png_unknown_chunk FAR * png_unknown_chunkp;
typedef png_unknown_chunk FAR * FAR * png_unknown_chunkpp;
#endif








































typedef struct png_info_struct
{
   
   png_uint_32 width;       
   png_uint_32 height;      
   png_uint_32 valid;       
   png_uint_32 rowbytes;    
   png_colorp palette;      
   png_uint_16 num_palette; 
   png_uint_16 num_trans;   
   png_byte bit_depth;      
   png_byte color_type;     
   
   png_byte compression_type; 
   png_byte filter_type;    
   png_byte interlace_type; 

   
   png_byte channels;       
   png_byte pixel_depth;    
   png_byte spare_byte;     
   png_byte signature[8];   

   





#if defined(PNG_gAMA_SUPPORTED) && defined(PNG_FLOATING_POINT_SUPPORTED)
   



   float gamma; 
#endif

#if defined(PNG_sRGB_SUPPORTED)
    
    
   png_byte srgb_intent; 
#endif

#if defined(PNG_TEXT_SUPPORTED)
   







   int num_text; 
   int max_text; 
   png_textp text; 
#endif 

#if defined(PNG_tIME_SUPPORTED)
   


   png_time mod_time;
#endif

#if defined(PNG_sBIT_SUPPORTED)
   





   png_color_8 sig_bit; 
#endif

#if defined(PNG_tRNS_SUPPORTED) || defined(PNG_READ_EXPAND_SUPPORTED) || \
defined(PNG_READ_BACKGROUND_SUPPORTED)
   








   png_bytep trans; 
   png_color_16 trans_values; 
#endif

#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   





   png_color_16 background;
#endif

#if defined(PNG_oFFs_SUPPORTED)
   




   png_int_32 x_offset; 
   png_int_32 y_offset; 
   png_byte offset_unit_type; 
#endif

#if defined(PNG_pHYs_SUPPORTED)
   



   png_uint_32 x_pixels_per_unit; 
   png_uint_32 y_pixels_per_unit; 
   png_byte phys_unit_type; 
#endif

#if defined(PNG_hIST_SUPPORTED)
   





   png_uint_16p hist;
#endif

#ifdef PNG_cHRM_SUPPORTED
   





#ifdef PNG_FLOATING_POINT_SUPPORTED
   float x_white;
   float y_white;
   float x_red;
   float y_red;
   float x_green;
   float y_green;
   float x_blue;
   float y_blue;
#endif
#endif

#if defined(PNG_pCAL_SUPPORTED)
   










   png_charp pcal_purpose;  
   png_int_32 pcal_X0;      
   png_int_32 pcal_X1;      
   png_charp pcal_units;    
   png_charpp pcal_params;  
   png_byte pcal_type;      
   png_byte pcal_nparams;   
#endif


#ifdef PNG_FREE_ME_SUPPORTED
   png_uint_32 free_me;     
#endif

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED) || \
 defined(PNG_HANDLE_AS_UNKNOWN_SUPPORTED)
   
   png_unknown_chunkp unknown_chunks;
   png_size_t unknown_chunks_num;
#endif

#if defined(PNG_iCCP_SUPPORTED)
   
   png_charp iccp_name;     
   png_charp iccp_profile;  
                            
   png_uint_32 iccp_proflen;  
   png_byte iccp_compression; 
#endif

#if defined(PNG_sPLT_SUPPORTED)
   
   png_sPLT_tp splt_palettes;
   png_uint_32 splt_palettes_num;
#endif

#if defined(PNG_sCAL_SUPPORTED)
   






   png_byte scal_unit;         
#ifdef PNG_FLOATING_POINT_SUPPORTED
   double scal_pixel_width;    
   double scal_pixel_height;   
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   png_charp scal_s_width;     
   png_charp scal_s_height;    
#endif
#endif

#if defined(PNG_INFO_IMAGE_SUPPORTED)
   
   
   png_bytepp row_pointers;        
#endif

#if defined(PNG_FIXED_POINT_SUPPORTED) && defined(PNG_gAMA_SUPPORTED)
   png_fixed_point int_gamma; 
#endif

#if defined(PNG_cHRM_SUPPORTED) && defined(PNG_FIXED_POINT_SUPPORTED)
   png_fixed_point int_x_white;
   png_fixed_point int_y_white;
   png_fixed_point int_x_red;
   png_fixed_point int_y_red;
   png_fixed_point int_x_green;
   png_fixed_point int_y_green;
   png_fixed_point int_x_blue;
   png_fixed_point int_y_blue;
#endif

#ifdef PNG_APNG_SUPPORTED
   png_uint_32 num_frames; 
   png_uint_32 num_plays;
   png_uint_32 next_frame_width;
   png_uint_32 next_frame_height;
   png_uint_32 next_frame_x_offset;
   png_uint_32 next_frame_y_offset;
   png_uint_16 next_frame_delay_num;
   png_uint_16 next_frame_delay_den;
   png_byte next_frame_dispose_op;
   png_byte next_frame_blend_op;
#endif

} png_info;

typedef png_info FAR * png_infop;
typedef png_info FAR * FAR * png_infopp;


#define PNG_UINT_31_MAX ((png_uint_32)0x7fffffffL)
#define PNG_UINT_32_MAX ((png_uint_32)(-1))
#define PNG_SIZE_MAX ((png_size_t)(-1))
#if defined(PNG_1_0_X) || defined (PNG_1_2_X)

#define PNG_MAX_UINT PNG_UINT_31_MAX
#endif



#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4


#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)

#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA


#define PNG_COMPRESSION_TYPE_BASE 0 /* Deflate method 8, 32K window */
#define PNG_COMPRESSION_TYPE_DEFAULT PNG_COMPRESSION_TYPE_BASE


#define PNG_FILTER_TYPE_BASE      0 /* Single row per-byte filtering */
#define PNG_INTRAPIXEL_DIFFERENCING 64 /* Used only in MNG datastreams */
#define PNG_FILTER_TYPE_DEFAULT   PNG_FILTER_TYPE_BASE


#define PNG_INTERLACE_NONE        0 /* Non-interlaced image */
#define PNG_INTERLACE_ADAM7       1 /* Adam7 interlacing */
#define PNG_INTERLACE_LAST        2 /* Not a valid value */


#define PNG_OFFSET_PIXEL          0 /* Offset in pixels */
#define PNG_OFFSET_MICROMETER     1 /* Offset in micrometers (1/10^6 meter) */
#define PNG_OFFSET_LAST           2 /* Not a valid value */


#define PNG_EQUATION_LINEAR       0 /* Linear transformation */
#define PNG_EQUATION_BASE_E       1 /* Exponential base e transform */
#define PNG_EQUATION_ARBITRARY    2 /* Arbitrary base exponential transform */
#define PNG_EQUATION_HYPERBOLIC   3 /* Hyperbolic sine transformation */
#define PNG_EQUATION_LAST         4 /* Not a valid value */


#define PNG_SCALE_UNKNOWN         0 /* unknown unit (image scale) */
#define PNG_SCALE_METER           1 /* meters per pixel */
#define PNG_SCALE_RADIAN          2 /* radians per pixel */
#define PNG_SCALE_LAST            3 /* Not a valid value */


#define PNG_RESOLUTION_UNKNOWN    0 /* pixels/unknown unit (aspect ratio) */
#define PNG_RESOLUTION_METER      1 /* pixels/meter */
#define PNG_RESOLUTION_LAST       2 /* Not a valid value */


#define PNG_sRGB_INTENT_PERCEPTUAL 0
#define PNG_sRGB_INTENT_RELATIVE   1
#define PNG_sRGB_INTENT_SATURATION 2
#define PNG_sRGB_INTENT_ABSOLUTE   3
#define PNG_sRGB_INTENT_LAST       4 /* Not a valid value */


#define PNG_KEYWORD_MAX_LENGTH     79


#define PNG_MAX_PALETTE_LENGTH    256






#define PNG_INFO_gAMA 0x0001
#define PNG_INFO_sBIT 0x0002
#define PNG_INFO_cHRM 0x0004
#define PNG_INFO_PLTE 0x0008
#define PNG_INFO_tRNS 0x0010
#define PNG_INFO_bKGD 0x0020
#define PNG_INFO_hIST 0x0040
#define PNG_INFO_pHYs 0x0080
#define PNG_INFO_oFFs 0x0100
#define PNG_INFO_tIME 0x0200
#define PNG_INFO_pCAL 0x0400
#define PNG_INFO_sRGB 0x0800   /* GR-P, 0.96a */
#define PNG_INFO_iCCP 0x1000   /* ESR, 1.0.6 */
#define PNG_INFO_sPLT 0x2000   /* ESR, 1.0.6 */
#define PNG_INFO_sCAL 0x4000   /* ESR, 1.0.6 */
#define PNG_INFO_IDAT 0x8000L  /* ESR, 1.0.6 */
#ifdef PNG_APNG_SUPPORTED
#define PNG_INFO_acTL 0x10000L
#define PNG_INFO_fcTL 0x20000L
#endif





typedef struct png_row_info_struct
{
   png_uint_32 width; 
   png_uint_32 rowbytes; 
   png_byte color_type; 
   png_byte bit_depth; 
   png_byte channels; 
   png_byte pixel_depth; 
} png_row_info;

typedef png_row_info FAR * png_row_infop;
typedef png_row_info FAR * FAR * png_row_infopp;







typedef struct png_struct_def png_struct;
typedef png_struct FAR * png_structp;

typedef void (PNGAPI *png_error_ptr) PNGARG((png_structp, png_const_charp));
typedef void (PNGAPI *png_rw_ptr) PNGARG((png_structp, png_bytep, png_size_t));
typedef void (PNGAPI *png_flush_ptr) PNGARG((png_structp));
typedef void (PNGAPI *png_read_status_ptr) PNGARG((png_structp, png_uint_32,
   int));
typedef void (PNGAPI *png_write_status_ptr) PNGARG((png_structp, png_uint_32,
   int));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
typedef void (PNGAPI *png_progressive_info_ptr) PNGARG((png_structp, png_infop));
typedef void (PNGAPI *png_progressive_end_ptr) PNGARG((png_structp, png_infop));
typedef void (PNGAPI *png_progressive_row_ptr) PNGARG((png_structp, png_bytep,
   png_uint_32, int));
#ifdef PNG_APNG_SUPPORTED
typedef void (PNGAPI *png_progressive_frame_ptr) PNGARG((png_structp, 
   png_uint_32));
#endif
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_LEGACY_SUPPORTED)
typedef void (PNGAPI *png_user_transform_ptr) PNGARG((png_structp,
    png_row_infop, png_bytep));
#endif

#if defined(PNG_USER_CHUNKS_SUPPORTED)
typedef int (PNGAPI *png_user_chunk_ptr) PNGARG((png_structp, png_unknown_chunkp));
#endif
#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
typedef void (PNGAPI *png_unknown_chunk_ptr) PNGARG((png_structp));
#endif


#define PNG_TRANSFORM_IDENTITY       0x0000    /* read and write */
#define PNG_TRANSFORM_STRIP_16       0x0001    /* read only */
#define PNG_TRANSFORM_STRIP_ALPHA    0x0002    /* read only */
#define PNG_TRANSFORM_PACKING        0x0004    /* read and write */
#define PNG_TRANSFORM_PACKSWAP       0x0008    /* read and write */
#define PNG_TRANSFORM_EXPAND         0x0010    /* read only */
#define PNG_TRANSFORM_INVERT_MONO    0x0020    /* read and write */
#define PNG_TRANSFORM_SHIFT          0x0040    /* read and write */
#define PNG_TRANSFORM_BGR            0x0080    /* read and write */
#define PNG_TRANSFORM_SWAP_ALPHA     0x0100    /* read and write */
#define PNG_TRANSFORM_SWAP_ENDIAN    0x0200    /* read and write */
#define PNG_TRANSFORM_INVERT_ALPHA   0x0400    /* read and write */
#define PNG_TRANSFORM_STRIP_FILLER   0x0800    /* write only, deprecated */

#define PNG_TRANSFORM_STRIP_FILLER_BEFORE 0x0800  /* write only */
#define PNG_TRANSFORM_STRIP_FILLER_AFTER  0x1000  /* write only */


#define PNG_FLAG_MNG_EMPTY_PLTE     0x01
#define PNG_FLAG_MNG_FILTER_64      0x04
#define PNG_ALL_MNG_FEATURES        0x05

typedef png_voidp (*png_malloc_ptr) PNGARG((png_structp, png_size_t));
typedef void (*png_free_ptr) PNGARG((png_structp, png_voidp));








struct png_struct_def
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf jmpbuf;            
#endif
   png_error_ptr error_fn;    
   png_error_ptr warning_fn;  
   png_voidp error_ptr;       
   png_rw_ptr write_data_fn;  
   png_rw_ptr read_data_fn;   
   png_voidp io_ptr;          

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED)
   png_user_transform_ptr read_user_transform_fn; 
#endif

#if defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
   png_user_transform_ptr write_user_transform_fn; 
#endif


#if defined(PNG_USER_TRANSFORM_PTR_SUPPORTED)
#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
   png_voidp user_transform_ptr; 
   png_byte user_transform_depth;    
   png_byte user_transform_channels; 
#endif
#endif

   png_uint_32 mode;          
   png_uint_32 flags;         
   png_uint_32 transformations; 

   z_stream zstream;          
   png_bytep zbuf;            
   png_size_t zbuf_size;      
   int zlib_level;            
   int zlib_method;           
   int zlib_window_bits;      
   int zlib_mem_level;        
   int zlib_strategy;         

   png_uint_32 width;         
   png_uint_32 height;        
   png_uint_32 num_rows;      
   png_uint_32 usr_width;     
   png_uint_32 rowbytes;      
   png_uint_32 irowbytes;     
   png_uint_32 iwidth;        
   png_uint_32 row_number;    
   png_bytep prev_row;        
   png_bytep row_buf;         
#ifndef PNG_NO_WRITE_FILTER
   png_bytep sub_row;         
   png_bytep up_row;          
   png_bytep avg_row;         
   png_bytep paeth_row;       
#endif
   png_row_info row_info;     

   png_uint_32 idat_size;     
   png_uint_32 crc;           
   png_colorp palette;        
   png_uint_16 num_palette;   
   png_uint_16 num_trans;     
   png_byte chunk_name[5];    
   png_byte compression;      
   png_byte filter;           
   png_byte interlaced;       
   png_byte pass;             
   png_byte do_filter;        
   png_byte color_type;       
   png_byte bit_depth;        
   png_byte usr_bit_depth;    
   png_byte pixel_depth;      
   png_byte channels;         
   png_byte usr_channels;     
   png_byte sig_bytes;        

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
#ifdef PNG_LEGACY_SUPPORTED
   png_byte filler;           
#else
   png_uint_16 filler;           
#endif
#endif

#if defined(PNG_bKGD_SUPPORTED)
   png_byte background_gamma_type;
#  ifdef PNG_FLOATING_POINT_SUPPORTED
   float background_gamma;
#  endif
   png_color_16 background;   
#if defined(PNG_READ_GAMMA_SUPPORTED)
   png_color_16 background_1; 
#endif
#endif 

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   png_flush_ptr output_flush_fn; 
   png_uint_32 flush_dist;    
   png_uint_32 flush_rows;    
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   int gamma_shift;      
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float gamma;          
   float screen_gamma;   
#endif
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep gamma_table;     
   png_bytep gamma_from_1;    
   png_bytep gamma_to_1;      
   png_uint_16pp gamma_16_table; 
   png_uint_16pp gamma_16_from_1; 
   png_uint_16pp gamma_16_to_1; 
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_sBIT_SUPPORTED)
   png_color_8 sig_bit;       
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
   png_color_8 shift;         
#endif

#if defined(PNG_tRNS_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) \
 || defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep trans;           
   png_color_16 trans_values; 
#endif

   png_read_status_ptr read_row_fn;   
   png_write_status_ptr write_row_fn; 
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_progressive_info_ptr info_fn; 
   png_progressive_row_ptr row_fn;   
   png_progressive_end_ptr end_fn;   
   png_bytep save_buffer_ptr;        
   png_bytep save_buffer;            
   png_bytep current_buffer_ptr;     
   png_bytep current_buffer;         
   png_uint_32 push_length;          
   png_uint_32 skip_length;          
   png_size_t save_buffer_size;      
   png_size_t save_buffer_max;       
   png_size_t buffer_size;           
   png_size_t current_buffer_size;   
   int process_mode;                 
   int cur_palette;                  

#  if defined(PNG_TEXT_SUPPORTED)
     png_size_t current_text_size;   
     png_size_t current_text_left;   
     png_charp current_text;         
     png_charp current_text_ptr;     
#  endif 
#endif 

#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)

   png_bytepp offset_table_ptr;
   png_bytep offset_table;
   png_uint_16 offset_table_number;
   png_uint_16 offset_table_count;
   png_uint_16 offset_table_count_free;
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
   png_bytep palette_lookup;         
   png_bytep dither_index;           
#endif

#if defined(PNG_READ_DITHER_SUPPORTED) || defined(PNG_hIST_SUPPORTED)
   png_uint_16p hist;                
#endif

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   png_byte heuristic_method;        
   png_byte num_prev_filters;        
   png_bytep prev_filters;           
   png_uint_16p filter_weights;      
   png_uint_16p inv_filter_weights;  
   png_uint_16p filter_costs;        
   png_uint_16p inv_filter_costs;    
#endif

#if defined(PNG_TIME_RFC1123_SUPPORTED)
   png_charp time_buffer;            
#endif



#ifdef PNG_FREE_ME_SUPPORTED
   png_uint_32 free_me;   
#endif

#if defined(PNG_USER_CHUNKS_SUPPORTED)
   png_voidp user_chunk_ptr;
   png_user_chunk_ptr read_user_chunk_fn; 
#endif

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   int num_chunk_list;
   png_bytep chunk_list;
#endif


#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   png_byte rgb_to_gray_status;
   
   png_uint_16 rgb_to_gray_red_coeff;
   png_uint_16 rgb_to_gray_green_coeff;
   png_uint_16 rgb_to_gray_blue_coeff;
#endif


#if defined(PNG_MNG_FEATURES_SUPPORTED) || \
    defined(PNG_READ_EMPTY_PLTE_SUPPORTED) || \
    defined(PNG_WRITE_EMPTY_PLTE_SUPPORTED)

#ifdef PNG_1_0_X
   png_byte mng_features_permitted;
#else
   png_uint_32 mng_features_permitted;
#endif 
#endif


#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_fixed_point int_gamma;
#endif


#if defined(PNG_MNG_FEATURES_SUPPORTED)
   png_byte filter_type;
#endif

#if defined(PNG_1_0_X)

   png_uint_32 row_buf_size;
#endif


#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
#  if !defined(PNG_1_0_X)
#    if defined(PNG_MMX_CODE_SUPPORTED)
   png_byte     mmx_bitdepth_threshold;
   png_uint_32  mmx_rowbytes_threshold;
#    endif
   png_uint_32  asm_flags;
#  endif
#endif


#ifdef PNG_USER_MEM_SUPPORTED
   png_voidp mem_ptr;            
   png_malloc_ptr malloc_fn;     
   png_free_ptr free_fn;         
#endif


   png_bytep big_row_buf;        

#if defined(PNG_READ_DITHER_SUPPORTED)

   png_bytep dither_sort;        
   png_bytep index_to_palette;   
                                 
   png_bytep palette_to_index;   
                                 
#endif


   png_byte compression_type;

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
   png_uint_32 user_width_max;
   png_uint_32 user_height_max;
#endif

#ifdef PNG_APNG_SUPPORTED
   png_uint_32 apng_flags;
   png_uint_32 next_seq_num;         
   png_uint_32 first_frame_width;
   png_uint_32 first_frame_height;

#ifdef PNG_READ_APNG_SUPPORTED
   png_uint_32 num_frames_read;      
                                     
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_progressive_frame_ptr frame_info_fn; 
   png_progressive_frame_ptr frame_end_fn;  
#endif
#endif

#ifdef PNG_WRITE_APNG_SUPPORTED
   png_uint_32 num_frames_to_write;
   png_uint_32 num_frames_written;
#endif


#define PNG_FIRST_FRAME_HIDDEN       0x0001


#define PNG_DISPOSE_OP_NONE        0x00
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#define PNG_DISPOSE_OP_PREVIOUS    0x02


#define PNG_BLEND_OP_SOURCE        0x00
#define PNG_BLEND_OP_OVER          0x01
#endif 


#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
   
   png_unknown_chunk unknown_chunk;
#endif


  png_uint_32 old_big_row_buf_size, old_prev_row_size;


  png_charp chunkdata;  

};





typedef png_structp version_1_2_40;

typedef png_struct FAR * FAR * png_structpp;








extern PNG_EXPORT(png_uint_32,png_access_version_number) PNGARG((void));




extern PNG_EXPORT(void,png_set_sig_bytes) PNGARG((png_structp png_ptr,
   int num_bytes));






extern PNG_EXPORT(int,png_sig_cmp) PNGARG((png_bytep sig, png_size_t start,
   png_size_t num_to_check));




extern PNG_EXPORT(int,png_check_sig) PNGARG((png_bytep sig, int num));


extern PNG_EXPORT(png_structp,png_create_read_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));


extern PNG_EXPORT(png_structp,png_create_write_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));

#ifdef PNG_WRITE_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_compression_buffer_size)
   PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_WRITE_SUPPORTED
extern PNG_EXPORT(void,png_set_compression_buffer_size)
   PNGARG((png_structp png_ptr, png_uint_32 size));
#endif


extern PNG_EXPORT(int,png_reset_zstream) PNGARG((png_structp png_ptr));


#ifdef PNG_USER_MEM_SUPPORTED
extern PNG_EXPORT(png_structp,png_create_read_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn));
extern PNG_EXPORT(png_structp,png_create_write_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn));
#endif


extern PNG_EXPORT(void,png_write_chunk) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_bytep data, png_size_t length));


extern PNG_EXPORT(void,png_write_chunk_start) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_uint_32 length));


extern PNG_EXPORT(void,png_write_chunk_data) PNGARG((png_structp png_ptr,
   png_bytep data, png_size_t length));


extern PNG_EXPORT(void,png_write_chunk_end) PNGARG((png_structp png_ptr));


extern PNG_EXPORT(png_infop,png_create_info_struct)
   PNGARG((png_structp png_ptr));

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)

extern PNG_EXPORT(void,png_info_init) PNGARG((png_infop info_ptr));
#undef png_info_init
#define png_info_init(info_ptr) png_info_init_3(&info_ptr,\
    png_sizeof(png_info));
#endif

extern PNG_EXPORT(void,png_info_init_3) PNGARG((png_infopp info_ptr,
    png_size_t png_info_struct_size));


extern PNG_EXPORT(void,png_write_info_before_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
extern PNG_EXPORT(void,png_write_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifndef PNG_NO_SEQUENTIAL_READ_SUPPORTED

extern PNG_EXPORT(void,png_read_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif

#if defined(PNG_TIME_RFC1123_SUPPORTED)
extern PNG_EXPORT(png_charp,png_convert_to_rfc1123)
   PNGARG((png_structp png_ptr, png_timep ptime));
#endif

#if !defined(_WIN32_WCE)

#if defined(PNG_WRITE_tIME_SUPPORTED)

extern PNG_EXPORT(void,png_convert_from_struct_tm) PNGARG((png_timep ptime,
   struct tm FAR * ttime));


extern PNG_EXPORT(void,png_convert_from_time_t) PNGARG((png_timep ptime,
   time_t ttime));
#endif 
#endif 

#if defined(PNG_READ_EXPAND_SUPPORTED)

extern PNG_EXPORT(void,png_set_expand) PNGARG((png_structp png_ptr));
#if !defined(PNG_1_0_X)
extern PNG_EXPORT(void,png_set_expand_gray_1_2_4_to_8) PNGARG((png_structp
  png_ptr));
#endif
extern PNG_EXPORT(void,png_set_palette_to_rgb) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(void,png_set_tRNS_to_alpha) PNGARG((png_structp png_ptr));
#if defined(PNG_1_0_X) || defined (PNG_1_2_X)

extern PNG_EXPORT(void,png_set_gray_1_2_4_to_8) PNGARG((png_structp png_ptr));
#endif
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)

extern PNG_EXPORT(void,png_set_bgr) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)

extern PNG_EXPORT(void,png_set_gray_to_rgb) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)

#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_rgb_to_gray) PNGARG((png_structp png_ptr,
   int error_action, double red, double green ));
#endif
extern PNG_EXPORT(void,png_set_rgb_to_gray_fixed) PNGARG((png_structp png_ptr,
   int error_action, png_fixed_point red, png_fixed_point green ));
extern PNG_EXPORT(png_byte,png_get_rgb_to_gray_status) PNGARG((png_structp
   png_ptr));
#endif

extern PNG_EXPORT(void,png_build_grayscale_palette) PNGARG((int bit_depth,
   png_colorp palette));

#if defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
extern PNG_EXPORT(void,png_set_strip_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED) || \
    defined(PNG_WRITE_SWAP_ALPHA_SUPPORTED)
extern PNG_EXPORT(void,png_set_swap_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED) || \
    defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
extern PNG_EXPORT(void,png_set_invert_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)

extern PNG_EXPORT(void,png_set_filler) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));

#define PNG_FILLER_BEFORE 0
#define PNG_FILLER_AFTER 1

#if !defined(PNG_1_0_X)
extern PNG_EXPORT(void,png_set_add_alpha) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));
#endif
#endif 

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)

extern PNG_EXPORT(void,png_set_swap) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_PACK_SUPPORTED) || defined(PNG_WRITE_PACK_SUPPORTED)

extern PNG_EXPORT(void,png_set_packing) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || defined(PNG_WRITE_PACKSWAP_SUPPORTED)

extern PNG_EXPORT(void,png_set_packswap) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)

extern PNG_EXPORT(void,png_set_shift) PNGARG((png_structp png_ptr,
   png_color_8p true_bits));
#endif

#if defined(PNG_READ_INTERLACING_SUPPORTED) || \
    defined(PNG_WRITE_INTERLACING_SUPPORTED)

extern PNG_EXPORT(int,png_set_interlace_handling) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)

extern PNG_EXPORT(void,png_set_invert_mono) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)

#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_background) PNGARG((png_structp png_ptr,
   png_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma));
#endif
#define PNG_BACKGROUND_GAMMA_UNKNOWN 0
#define PNG_BACKGROUND_GAMMA_SCREEN  1
#define PNG_BACKGROUND_GAMMA_FILE    2
#define PNG_BACKGROUND_GAMMA_UNIQUE  3
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)

extern PNG_EXPORT(void,png_set_strip_16) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)

extern PNG_EXPORT(void,png_set_dither) PNGARG((png_structp png_ptr,
   png_colorp palette, int num_palette, int maximum_colors,
   png_uint_16p histogram, int full_dither));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)

#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_gamma) PNGARG((png_structp png_ptr,
   double screen_gamma, double default_file_gamma));
#endif
#endif

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)
#if defined(PNG_READ_EMPTY_PLTE_SUPPORTED) || \
    defined(PNG_WRITE_EMPTY_PLTE_SUPPORTED)


extern PNG_EXPORT(void,png_permit_empty_plte) PNGARG((png_structp png_ptr,
   int empty_plte_permitted));
#endif
#endif

#if defined(PNG_WRITE_FLUSH_SUPPORTED)

extern PNG_EXPORT(void,png_set_flush) PNGARG((png_structp png_ptr, int nrows));

extern PNG_EXPORT(void,png_write_flush) PNGARG((png_structp png_ptr));
#endif


extern PNG_EXPORT(void,png_start_read_image) PNGARG((png_structp png_ptr));


extern PNG_EXPORT(void,png_read_update_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifndef PNG_NO_SEQUENTIAL_READ_SUPPORTED

extern PNG_EXPORT(void,png_read_rows) PNGARG((png_structp png_ptr,
   png_bytepp row, png_bytepp display_row, png_uint_32 num_rows));
#endif

#ifndef PNG_NO_SEQUENTIAL_READ_SUPPORTED

extern PNG_EXPORT(void,png_read_row) PNGARG((png_structp png_ptr,
   png_bytep row,
   png_bytep display_row));
#endif

#ifndef PNG_NO_SEQUENTIAL_READ_SUPPORTED

extern PNG_EXPORT(void,png_read_image) PNGARG((png_structp png_ptr,
   png_bytepp image));
#endif


extern PNG_EXPORT(void,png_write_row) PNGARG((png_structp png_ptr,
   png_bytep row));


extern PNG_EXPORT(void,png_write_rows) PNGARG((png_structp png_ptr,
   png_bytepp row, png_uint_32 num_rows));


extern PNG_EXPORT(void,png_write_image) PNGARG((png_structp png_ptr,
   png_bytepp image));

#ifdef PNG_WRITE_APNG_SUPPORTED
extern PNG_EXPORT (void,png_write_frame_head) PNGARG((png_structp png_ptr,
   png_infop png_info, png_bytepp row_pointers,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset, 
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op));

extern PNG_EXPORT (void,png_write_frame_tail) PNGARG((png_structp png_ptr,
   png_infop png_info));
#endif
   

extern PNG_EXPORT(void,png_write_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifndef PNG_NO_SEQUENTIAL_READ_SUPPORTED

extern PNG_EXPORT(void,png_read_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif


extern PNG_EXPORT(void,png_destroy_info_struct) PNGARG((png_structp png_ptr,
   png_infopp info_ptr_ptr));


extern PNG_EXPORT(void,png_destroy_read_struct) PNGARG((png_structpp
   png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr));


extern void png_read_destroy PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_infop end_info_ptr));


extern PNG_EXPORT(void,png_destroy_write_struct)
   PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr));


extern void png_write_destroy PNGARG((png_structp png_ptr));


extern PNG_EXPORT(void,png_set_crc_action) PNGARG((png_structp png_ptr,
   int crit_action, int ancil_action));










#define PNG_CRC_DEFAULT       0  /* error/quit          warn/discard data */
#define PNG_CRC_ERROR_QUIT    1  /* error/quit          error/quit        */
#define PNG_CRC_WARN_DISCARD  2  /* (INVALID)           warn/discard data */
#define PNG_CRC_WARN_USE      3  /* warn/use data       warn/use data     */
#define PNG_CRC_QUIET_USE     4  /* quiet/use data      quiet/use data    */
#define PNG_CRC_NO_CHANGE     5  /* use current value   use current value */












extern PNG_EXPORT(void,png_set_filter) PNGARG((png_structp png_ptr, int method,
   int filters));






#define PNG_NO_FILTERS     0x00
#define PNG_FILTER_NONE    0x08
#define PNG_FILTER_SUB     0x10
#define PNG_FILTER_UP      0x20
#define PNG_FILTER_AVG     0x40
#define PNG_FILTER_PAETH   0x80
#define PNG_ALL_FILTERS (PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | \
                         PNG_FILTER_AVG | PNG_FILTER_PAETH)




#define PNG_FILTER_VALUE_NONE  0
#define PNG_FILTER_VALUE_SUB   1
#define PNG_FILTER_VALUE_UP    2
#define PNG_FILTER_VALUE_AVG   3
#define PNG_FILTER_VALUE_PAETH 4
#define PNG_FILTER_VALUE_LAST  5

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED) 




























#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_filter_heuristics) PNGARG((png_structp png_ptr,
   int heuristic_method, int num_weights, png_doublep filter_weights,
   png_doublep filter_costs));
#endif
#endif 




#define PNG_FILTER_HEURISTIC_DEFAULT    0  /* Currently "UNWEIGHTED" */
#define PNG_FILTER_HEURISTIC_UNWEIGHTED 1  /* Used by libpng < 0.95 */
#define PNG_FILTER_HEURISTIC_WEIGHTED   2  /* Experimental feature */
#define PNG_FILTER_HEURISTIC_LAST       3  /* Not a valid value */








extern PNG_EXPORT(void,png_set_compression_level) PNGARG((png_structp png_ptr,
   int level));

extern PNG_EXPORT(void,png_set_compression_mem_level)
   PNGARG((png_structp png_ptr, int mem_level));

extern PNG_EXPORT(void,png_set_compression_strategy)
   PNGARG((png_structp png_ptr, int strategy));

extern PNG_EXPORT(void,png_set_compression_window_bits)
   PNGARG((png_structp png_ptr, int window_bits));

extern PNG_EXPORT(void,png_set_compression_method) PNGARG((png_structp png_ptr,
   int method));










#if !defined(PNG_NO_STDIO)

extern PNG_EXPORT(void,png_init_io) PNGARG((png_structp png_ptr, png_FILE_p fp));
#endif









extern PNG_EXPORT(void,png_set_error_fn) PNGARG((png_structp png_ptr,
   png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warning_fn));


extern PNG_EXPORT(png_voidp,png_get_error_ptr) PNGARG((png_structp png_ptr));











extern PNG_EXPORT(void,png_set_write_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn));


extern PNG_EXPORT(void,png_set_read_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr read_data_fn));


extern PNG_EXPORT(png_voidp,png_get_io_ptr) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_read_status_fn) PNGARG((png_structp png_ptr,
   png_read_status_ptr read_row_fn));

extern PNG_EXPORT(void,png_set_write_status_fn) PNGARG((png_structp png_ptr,
   png_write_status_ptr write_row_fn));

#ifdef PNG_USER_MEM_SUPPORTED

extern PNG_EXPORT(void,png_set_mem_fn) PNGARG((png_structp png_ptr,
   png_voidp mem_ptr, png_malloc_ptr malloc_fn, png_free_ptr free_fn));

extern PNG_EXPORT(png_voidp,png_get_mem_ptr) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_LEGACY_SUPPORTED)
extern PNG_EXPORT(void,png_set_read_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr read_user_transform_fn));
#endif

#if defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_LEGACY_SUPPORTED)
extern PNG_EXPORT(void,png_set_write_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr write_user_transform_fn));
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_LEGACY_SUPPORTED)
extern PNG_EXPORT(void,png_set_user_transform_info) PNGARG((png_structp
   png_ptr, png_voidp user_transform_ptr, int user_transform_depth,
   int user_transform_channels));

extern PNG_EXPORT(png_voidp,png_get_user_transform_ptr)
   PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_USER_CHUNKS_SUPPORTED
extern PNG_EXPORT(void,png_set_read_user_chunk_fn) PNGARG((png_structp png_ptr,
   png_voidp user_chunk_ptr, png_user_chunk_ptr read_user_chunk_fn));
extern PNG_EXPORT(png_voidp,png_get_user_chunk_ptr) PNGARG((png_structp
   png_ptr));
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED



extern PNG_EXPORT(void,png_set_progressive_read_fn) PNGARG((png_structp png_ptr,
   png_voidp progressive_ptr,
   png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
   png_progressive_end_ptr end_fn));
#ifdef PNG_READ_APNG_SUPPORTED
extern PNG_EXPORT(void,png_set_progressive_frame_fn) PNGARG((png_structp png_ptr,
   png_progressive_frame_ptr frame_info_fn,
   png_progressive_frame_ptr frame_end_fn));
#endif


extern PNG_EXPORT(png_voidp,png_get_progressive_ptr)
   PNGARG((png_structp png_ptr));


extern PNG_EXPORT(void,png_process_data) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep buffer, png_size_t buffer_size));




extern PNG_EXPORT(void,png_progressive_combine_row) PNGARG((png_structp png_ptr,
   png_bytep old_row, png_bytep new_row));
#endif 

extern PNG_EXPORT(png_voidp,png_malloc) PNGARG((png_structp png_ptr,
   png_uint_32 size));

#if defined(PNG_1_0_X)
#  define png_malloc_warn png_malloc
#else

extern PNG_EXPORT(png_voidp,png_malloc_warn) PNGARG((png_structp png_ptr,
   png_uint_32 size));
#endif


extern PNG_EXPORT(void,png_free) PNGARG((png_structp png_ptr, png_voidp ptr));

#if defined(PNG_1_0_X)

extern PNG_EXPORT(voidpf,png_zalloc) PNGARG((voidpf png_ptr, uInt items,
   uInt size));


extern PNG_EXPORT(void,png_zfree) PNGARG((voidpf png_ptr, voidpf ptr));
#endif


extern PNG_EXPORT(void,png_free_data) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 free_me, int num));
#ifdef PNG_FREE_ME_SUPPORTED



extern PNG_EXPORT(void,png_data_freer) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int freer, png_uint_32 mask));
#endif

#define PNG_DESTROY_WILL_FREE_DATA 1
#define PNG_SET_WILL_FREE_DATA 1
#define PNG_USER_WILL_FREE_DATA 2

#define PNG_FREE_HIST 0x0008
#define PNG_FREE_ICCP 0x0010
#define PNG_FREE_SPLT 0x0020
#define PNG_FREE_ROWS 0x0040
#define PNG_FREE_PCAL 0x0080
#define PNG_FREE_SCAL 0x0100
#define PNG_FREE_UNKN 0x0200
#define PNG_FREE_LIST 0x0400
#define PNG_FREE_PLTE 0x1000
#define PNG_FREE_TRNS 0x2000
#define PNG_FREE_TEXT 0x4000
#define PNG_FREE_ALL  0x7fff
#define PNG_FREE_MUL  0x4220 /* PNG_FREE_SPLT|PNG_FREE_TEXT|PNG_FREE_UNKN */

#ifdef PNG_USER_MEM_SUPPORTED
extern PNG_EXPORT(png_voidp,png_malloc_default) PNGARG((png_structp png_ptr,
   png_uint_32 size));
extern PNG_EXPORT(void,png_free_default) PNGARG((png_structp png_ptr,
   png_voidp ptr));
#endif

extern PNG_EXPORT(png_voidp,png_memcpy_check) PNGARG((png_structp png_ptr,
   png_voidp s1, png_voidp s2, png_uint_32 size));

extern PNG_EXPORT(png_voidp,png_memset_check) PNGARG((png_structp png_ptr,
   png_voidp s1, int value, png_uint_32 size));

#if defined(USE_FAR_KEYWORD)  
extern void *png_far_to_near PNGARG((png_structp png_ptr,png_voidp ptr,
   int check));
#endif 

#ifndef PNG_NO_ERROR_TEXT

extern PNG_EXPORT(void,png_error) PNGARG((png_structp png_ptr,
   png_const_charp error_message));


extern PNG_EXPORT(void,png_chunk_error) PNGARG((png_structp png_ptr,
   png_const_charp error_message));
#else

extern PNG_EXPORT(void,png_err) PNGARG((png_structp png_ptr));
#endif

#ifndef PNG_NO_WARNINGS

extern PNG_EXPORT(void,png_warning) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));

#ifdef PNG_READ_SUPPORTED

extern PNG_EXPORT(void,png_chunk_warning) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));
#endif 
#endif 














extern PNG_EXPORT(png_uint_32,png_get_valid) PNGARG((png_structp png_ptr,
png_infop info_ptr, png_uint_32 flag));


extern PNG_EXPORT(png_uint_32,png_get_rowbytes) PNGARG((png_structp png_ptr,
png_infop info_ptr));

#if defined(PNG_INFO_IMAGE_SUPPORTED)



extern PNG_EXPORT(png_bytepp,png_get_rows) PNGARG((png_structp png_ptr,
png_infop info_ptr));



extern PNG_EXPORT(void,png_set_rows) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers));
#endif


extern PNG_EXPORT(png_byte,png_get_channels) PNGARG((png_structp png_ptr,
png_infop info_ptr));

#ifdef PNG_EASY_ACCESS_SUPPORTED

extern PNG_EXPORT(png_uint_32, png_get_image_width) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_uint_32, png_get_image_height) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_byte, png_get_bit_depth) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_byte, png_get_color_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_byte, png_get_filter_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_byte, png_get_interlace_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_byte, png_get_compression_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));


extern PNG_EXPORT(png_uint_32, png_get_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32, png_get_x_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32, png_get_y_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));


#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(float, png_get_pixel_aspect_ratio) PNGARG((png_structp
png_ptr, png_infop info_ptr));
#endif


extern PNG_EXPORT(png_int_32, png_get_x_offset_pixels) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_int_32, png_get_y_offset_pixels) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_int_32, png_get_x_offset_microns) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_int_32, png_get_y_offset_microns) PNGARG((png_structp
png_ptr, png_infop info_ptr));

#endif 


extern PNG_EXPORT(png_bytep,png_get_signature) PNGARG((png_structp png_ptr,
png_infop info_ptr));

#if defined(PNG_bKGD_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p *background));
#endif

#if defined(PNG_bKGD_SUPPORTED)
extern PNG_EXPORT(void,png_set_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p background));
#endif

#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double *white_x, double *white_y, double *red_x,
   double *red_y, double *green_x, double *green_y, double *blue_x,
   double *blue_y));
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_cHRM_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point *int_white_x, png_fixed_point
   *int_white_y, png_fixed_point *int_red_x, png_fixed_point *int_red_y,
   png_fixed_point *int_green_x, png_fixed_point *int_green_y, png_fixed_point
   *int_blue_x, png_fixed_point *int_blue_y));
#endif
#endif

#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double white_x, double white_y, double red_x,
   double red_y, double green_x, double green_y, double blue_x, double blue_y));
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_cHRM_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point int_white_x, png_fixed_point int_white_y,
   png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
   int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
   png_fixed_point int_blue_y));
#endif
#endif

#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double *file_gamma));
#endif
extern PNG_EXPORT(png_uint_32,png_get_gAMA_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point *int_file_gamma));
#endif

#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double file_gamma));
#endif
extern PNG_EXPORT(void,png_set_gAMA_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point int_file_gamma));
#endif

#if defined(PNG_hIST_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p *hist));
#endif

#if defined(PNG_hIST_SUPPORTED)
extern PNG_EXPORT(void,png_set_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p hist));
#endif

extern PNG_EXPORT(png_uint_32,png_get_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *width, png_uint_32 *height,
   int *bit_depth, int *color_type, int *interlace_method,
   int *compression_method, int *filter_method));

extern PNG_EXPORT(void,png_set_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_method, int compression_method,
   int filter_method));

#if defined(PNG_oFFs_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_int_32 *offset_x, png_int_32 *offset_y,
   int *unit_type));
#endif

#if defined(PNG_oFFs_SUPPORTED)
extern PNG_EXPORT(void,png_set_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_int_32 offset_x, png_int_32 offset_y,
   int unit_type));
#endif

#if defined(PNG_pCAL_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp *purpose, png_int_32 *X0, png_int_32 *X1,
   int *type, int *nparams, png_charp *units, png_charpp *params));
#endif

#if defined(PNG_pCAL_SUPPORTED)
extern PNG_EXPORT(void,png_set_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp purpose, png_int_32 X0, png_int_32 X1,
   int type, int nparams, png_charp units, png_charpp params));
#endif

#if defined(PNG_pHYs_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type));
#endif

#if defined(PNG_pHYs_SUPPORTED)
extern PNG_EXPORT(void,png_set_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 res_x, png_uint_32 res_y, int unit_type));
#endif

extern PNG_EXPORT(png_uint_32,png_get_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp *palette, int *num_palette));

extern PNG_EXPORT(void,png_set_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp palette, int num_palette));

#if defined(PNG_sBIT_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p *sig_bit));
#endif

#if defined(PNG_sBIT_SUPPORTED)
extern PNG_EXPORT(void,png_set_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p sig_bit));
#endif

#if defined(PNG_sRGB_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int *intent));
#endif

#if defined(PNG_sRGB_SUPPORTED)
extern PNG_EXPORT(void,png_set_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
extern PNG_EXPORT(void,png_set_sRGB_gAMA_and_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
#endif

#if defined(PNG_iCCP_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_iCCP) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charpp name, int *compression_type,
   png_charpp profile, png_uint_32 *proflen));
   
#endif

#if defined(PNG_iCCP_SUPPORTED)
extern PNG_EXPORT(void,png_set_iCCP) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp name, int compression_type,
   png_charp profile, png_uint_32 proflen));
   
#endif

#if defined(PNG_sPLT_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_sPLT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_sPLT_tpp entries));
#endif

#if defined(PNG_sPLT_SUPPORTED)
extern PNG_EXPORT(void,png_set_sPLT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_sPLT_tp entries, int nentries));
#endif

#if defined(PNG_TEXT_SUPPORTED)

extern PNG_EXPORT(png_uint_32,png_get_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp *text_ptr, int *num_text));
#endif









#if defined(PNG_TEXT_SUPPORTED)
extern PNG_EXPORT(void,png_set_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp text_ptr, int num_text));
#endif

#if defined(PNG_tIME_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep *mod_time));
#endif

#if defined(PNG_tIME_SUPPORTED)
extern PNG_EXPORT(void,png_set_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep mod_time));
#endif

#if defined(PNG_tRNS_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep *trans, int *num_trans,
   png_color_16p *trans_values));
#endif

#if defined(PNG_tRNS_SUPPORTED)
extern PNG_EXPORT(void,png_set_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep trans, int num_trans,
   png_color_16p trans_values));
#endif

#if defined(PNG_tRNS_SUPPORTED)
#endif

#if defined(PNG_sCAL_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_sCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int *unit, double *width, double *height));
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_sCAL_s) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int *unit, png_charpp swidth, png_charpp sheight));
#endif
#endif
#endif 

#if defined(PNG_sCAL_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_sCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int unit, double width, double height));
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
extern PNG_EXPORT(void,png_set_sCAL_s) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int unit, png_charp swidth, png_charp sheight));
#endif
#endif
#endif 

#ifdef PNG_APNG_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_get_acTL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *num_frames, png_uint_32 *num_plays));
extern PNG_EXPORT(png_uint_32,png_set_acTL) PNGARG((png_structp png_ptr, 
   png_infop info_ptr, png_uint_32 num_frames, png_uint_32 num_plays));
extern PNG_EXPORT(png_uint_32,png_get_num_frames) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
extern PNG_EXPORT(png_uint_32,png_get_num_plays) 
   PNGARG((png_structp png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_uint_32,png_get_next_frame_fcTL) 
   PNGARG((png_structp png_ptr, png_infop info_ptr, png_uint_32 *width, 
   png_uint_32 *height, png_uint_32 *x_offset, png_uint_32 *y_offset, 
   png_uint_16 *delay_num, png_uint_16 *delay_den, png_byte *dispose_op,
   png_byte *blend_op));
extern PNG_EXPORT(png_uint_32,png_set_next_frame_fcTL) 
   PNGARG((png_structp png_ptr, png_infop info_ptr, png_uint_32 width, 
   png_uint_32 height, png_uint_32 x_offset, png_uint_32 y_offset, 
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op));
extern PNG_EXPORT(void,png_ensure_fcTL_is_valid)
   PNGARG((png_structp png_ptr,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den,
   png_byte dispose_op, png_byte blend_op));
extern PNG_EXPORT(png_uint_32,png_get_next_frame_width)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32,png_get_next_frame_height)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32,png_get_next_frame_x_offset)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32,png_get_next_frame_y_offset)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_16,png_get_next_frame_delay_num)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_16,png_get_next_frame_delay_den)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_byte,png_get_next_frame_dispose_op)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_byte,png_get_next_frame_blend_op)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_byte,png_get_first_frame_is_hidden)
   PNGARG((png_structp png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32,png_set_first_frame_is_hidden)
   PNGARG((png_structp png_ptr, png_infop info_ptr, png_byte is_hidden));
#endif 

#ifdef PNG_READ_APNG_SUPPORTED
extern PNG_EXPORT(void,png_read_frame_head) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED









extern PNG_EXPORT(void, png_set_keep_unknown_chunks) PNGARG((png_structp
   png_ptr, int keep, png_bytep chunk_list, int num_chunks));
PNG_EXPORT(int,png_handle_as_unknown) PNGARG((png_structp png_ptr, png_bytep
   chunk_name));
#endif
#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
extern PNG_EXPORT(void, png_set_unknown_chunks) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_unknown_chunkp unknowns, int num_unknowns));
extern PNG_EXPORT(void, png_set_unknown_chunk_location)
   PNGARG((png_structp png_ptr, png_infop info_ptr, int chunk, int location));
extern PNG_EXPORT(png_uint_32,png_get_unknown_chunks) PNGARG((png_structp
   png_ptr, png_infop info_ptr, png_unknown_chunkpp entries));
#endif





extern PNG_EXPORT(void, png_set_invalid) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int mask));

#if defined(PNG_INFO_IMAGE_SUPPORTED)

extern PNG_EXPORT(void, png_read_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
extern PNG_EXPORT(void, png_write_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
#endif






#ifdef PNG_DEBUG
#if (PNG_DEBUG > 0)
#if !defined(PNG_DEBUG_FILE) && defined(_MSC_VER)
#include <crtdbg.h>
#if (PNG_DEBUG > 1)
#ifndef _DEBUG
#  define _DEBUG
#endif
#ifndef png_debug
#define png_debug(l,m)  _RPT0(_CRT_WARN,m PNG_STRING_NEWLINE)
#endif
#ifndef png_debug1
#define png_debug1(l,m,p1)  _RPT1(_CRT_WARN,m PNG_STRING_NEWLINE,p1)
#endif
#ifndef png_debug2
#define png_debug2(l,m,p1,p2) _RPT2(_CRT_WARN,m PNG_STRING_NEWLINE,p1,p2)
#endif
#endif
#else 
#ifndef PNG_DEBUG_FILE
#define PNG_DEBUG_FILE stderr
#endif 

#if (PNG_DEBUG > 1)



#  ifdef __STDC__
#    ifndef png_debug
#      define png_debug(l,m) \
       { \
       int num_tabs=l; \
       fprintf(PNG_DEBUG_FILE,"%s"m PNG_STRING_NEWLINE,(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":"")))); \
       }
#    endif
#    ifndef png_debug1
#      define png_debug1(l,m,p1) \
       { \
       int num_tabs=l; \
       fprintf(PNG_DEBUG_FILE,"%s"m PNG_STRING_NEWLINE,(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))),p1); \
       }
#    endif
#    ifndef png_debug2
#      define png_debug2(l,m,p1,p2) \
       { \
       int num_tabs=l; \
       fprintf(PNG_DEBUG_FILE,"%s"m PNG_STRING_NEWLINE,(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))),p1,p2); \
       }
#    endif
#  else 
#    ifndef png_debug
#      define png_debug(l,m) \
       { \
       int num_tabs=l; \
       char format[256]; \
       snprintf(format,256,"%s%s%s",(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))), \
         m,PNG_STRING_NEWLINE); \
       fprintf(PNG_DEBUG_FILE,format); \
       }
#    endif
#    ifndef png_debug1
#      define png_debug1(l,m,p1) \
       { \
       int num_tabs=l; \
       char format[256]; \
       snprintf(format,256,"%s%s%s",(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))), \
         m,PNG_STRING_NEWLINE); \
       fprintf(PNG_DEBUG_FILE,format,p1); \
       }
#    endif
#    ifndef png_debug2
#      define png_debug2(l,m,p1,p2) \
       { \
       int num_tabs=l; \
       char format[256]; \
       snprintf(format,256,"%s%s%s",(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))), \
         m,PNG_STRING_NEWLINE); \
       fprintf(PNG_DEBUG_FILE,format,p1,p2); \
       }
#    endif
#  endif 
#endif 

#endif 
#endif 
#endif 
#ifndef png_debug
#define png_debug(l, m)
#endif
#ifndef png_debug1
#define png_debug1(l, m, p1)
#endif
#ifndef png_debug2
#define png_debug2(l, m, p1, p2)
#endif

extern PNG_EXPORT(png_charp,png_get_copyright) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(png_charp,png_get_header_ver) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(png_charp,png_get_header_version) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(png_charp,png_get_libpng_ver) PNGARG((png_structp png_ptr));

#ifdef PNG_MNG_FEATURES_SUPPORTED
extern PNG_EXPORT(png_uint_32,png_permit_mng_features) PNGARG((png_structp
   png_ptr, png_uint_32 mng_features_permitted));
#endif


#define PNG_HANDLE_CHUNK_AS_DEFAULT   0
#define PNG_HANDLE_CHUNK_NEVER        1
#define PNG_HANDLE_CHUNK_IF_SAFE      2
#define PNG_HANDLE_CHUNK_ALWAYS       3


#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
#if defined(PNG_MMX_CODE_SUPPORTED)
#define PNG_ASM_FLAG_MMX_SUPPORT_COMPILED  0x01  /* not user-settable */
#define PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU    0x02  /* not user-settable */
#define PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  0x04
#define PNG_ASM_FLAG_MMX_READ_INTERLACE    0x08
#define PNG_ASM_FLAG_MMX_READ_FILTER_SUB   0x10
#define PNG_ASM_FLAG_MMX_READ_FILTER_UP    0x20
#define PNG_ASM_FLAG_MMX_READ_FILTER_AVG   0x40
#define PNG_ASM_FLAG_MMX_READ_FILTER_PAETH 0x80
#define PNG_ASM_FLAGS_INITIALIZED          0x80000000  /* not user-settable */

#define PNG_MMX_READ_FLAGS ( PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  \
                           | PNG_ASM_FLAG_MMX_READ_INTERLACE    \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_SUB   \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_UP    \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_AVG   \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_PAETH )
#define PNG_MMX_WRITE_FLAGS ( 0 )

#define PNG_MMX_FLAGS ( PNG_ASM_FLAG_MMX_SUPPORT_COMPILED \
                      | PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU   \
                      | PNG_MMX_READ_FLAGS                \
                      | PNG_MMX_WRITE_FLAGS )

#define PNG_SELECT_READ   1
#define PNG_SELECT_WRITE  2
#endif 

#if !defined(PNG_1_0_X)

extern PNG_EXPORT(png_uint_32,png_get_mmx_flagmask)
   PNGARG((int flag_select, int *compilerID));


extern PNG_EXPORT(png_uint_32,png_get_asm_flagmask)
   PNGARG((int flag_select));


extern PNG_EXPORT(png_uint_32,png_get_asm_flags)
   PNGARG((png_structp png_ptr));


extern PNG_EXPORT(png_byte,png_get_mmx_bitdepth_threshold)
   PNGARG((png_structp png_ptr));


extern PNG_EXPORT(png_uint_32,png_get_mmx_rowbytes_threshold)
   PNGARG((png_structp png_ptr));


extern PNG_EXPORT(void,png_set_asm_flags)
   PNGARG((png_structp png_ptr, png_uint_32 asm_flags));


extern PNG_EXPORT(void,png_set_mmx_thresholds)
   PNGARG((png_structp png_ptr, png_byte mmx_bitdepth_threshold,
   png_uint_32 mmx_rowbytes_threshold));

#endif 

#if !defined(PNG_1_0_X)

extern PNG_EXPORT(int,png_mmx_support) PNGARG((void));
#endif 
#endif 




#ifdef PNG_ERROR_NUMBERS_SUPPORTED
extern PNG_EXPORT(void,png_set_strip_error_numbers) PNGARG((png_structp
   png_ptr, png_uint_32 strip_mode));
#endif


#ifdef PNG_SET_USER_LIMITS_SUPPORTED
extern PNG_EXPORT(void,png_set_user_limits) PNGARG((png_structp
   png_ptr, png_uint_32 user_width_max, png_uint_32 user_height_max));
extern PNG_EXPORT(png_uint_32,png_get_user_width_max) PNGARG((png_structp
   png_ptr));
extern PNG_EXPORT(png_uint_32,png_get_user_height_max) PNGARG((png_structp
   png_ptr));
#endif






#ifdef PNG_READ_COMPOSITE_NODIV_SUPPORTED












 

#  define png_composite(composite, fg, alpha, bg)                            \
     { png_uint_16 temp = (png_uint_16)((png_uint_16)(fg) * (png_uint_16)(alpha) \
                        +        (png_uint_16)(bg)*(png_uint_16)(255 -       \
                        (png_uint_16)(alpha)) + (png_uint_16)128);           \
       (composite) = (png_byte)((temp + (temp >> 8)) >> 8); }

#  define png_composite_16(composite, fg, alpha, bg)                         \
     { png_uint_32 temp = (png_uint_32)((png_uint_32)(fg) * (png_uint_32)(alpha) \
                        + (png_uint_32)(bg)*(png_uint_32)(65535L -           \
                        (png_uint_32)(alpha)) + (png_uint_32)32768L);        \
       (composite) = (png_uint_16)((temp + (temp >> 16)) >> 16); }

#else  

#  define png_composite(composite, fg, alpha, bg)                            \
     (composite) = (png_byte)(((png_uint_16)(fg) * (png_uint_16)(alpha) +    \
       (png_uint_16)(bg) * (png_uint_16)(255 - (png_uint_16)(alpha)) +       \
       (png_uint_16)127) / 255)

#  define png_composite_16(composite, fg, alpha, bg)                         \
     (composite) = (png_uint_16)(((png_uint_32)(fg) * (png_uint_32)(alpha) + \
       (png_uint_32)(bg)*(png_uint_32)(65535L - (png_uint_32)(alpha)) +      \
       (png_uint_32)32767) / (png_uint_32)65535L)

#endif 









#if defined(PNG_READ_BIG_ENDIAN_SUPPORTED)
#  define png_get_uint_32(buf) ( *((png_uint_32p) (buf)))
#  define png_get_uint_16(buf) ( *((png_uint_16p) (buf)))
#  define png_get_int_32(buf)  ( *((png_int_32p)  (buf)))
#else
extern PNG_EXPORT(png_uint_32,png_get_uint_32) PNGARG((png_bytep buf));
extern PNG_EXPORT(png_uint_16,png_get_uint_16) PNGARG((png_bytep buf));
extern PNG_EXPORT(png_int_32,png_get_int_32) PNGARG((png_bytep buf));
#endif 
extern PNG_EXPORT(png_uint_32,png_get_uint_31)
  PNGARG((png_structp png_ptr, png_bytep buf));




extern PNG_EXPORT(void,png_save_uint_32)
   PNGARG((png_bytep buf, png_uint_32 i));
extern PNG_EXPORT(void,png_save_int_32)
   PNGARG((png_bytep buf, png_int_32 i));





extern PNG_EXPORT(void,png_save_uint_16)
   PNGARG((png_bytep buf, unsigned int i));














#define PNG_HAVE_IHDR               0x01
#define PNG_HAVE_PLTE               0x02
#define PNG_HAVE_IDAT               0x04
#define PNG_AFTER_IDAT              0x08 /* Have complete zlib datastream */
#define PNG_HAVE_IEND               0x10

#if defined(PNG_INTERNAL)




#define PNG_HAVE_gAMA               0x20
#define PNG_HAVE_cHRM               0x40
#define PNG_HAVE_sRGB               0x80
#define PNG_HAVE_CHUNK_HEADER      0x100
#define PNG_WROTE_tIME             0x200
#define PNG_WROTE_INFO_BEFORE_PLTE 0x400
#define PNG_BACKGROUND_IS_GRAY     0x800
#define PNG_HAVE_PNG_SIGNATURE    0x1000
#define PNG_HAVE_CHUNK_AFTER_IDAT 0x2000 /* Have another chunk after IDAT */
#ifdef PNG_APNG_SUPPORTED
#define PNG_HAVE_acTL             0x4000
#define PNG_HAVE_fcTL             0x8000L
#endif


#define PNG_BGR                0x0001
#define PNG_INTERLACE          0x0002
#define PNG_PACK               0x0004
#define PNG_SHIFT              0x0008
#define PNG_SWAP_BYTES         0x0010
#define PNG_INVERT_MONO        0x0020
#define PNG_DITHER             0x0040
#define PNG_BACKGROUND         0x0080
#define PNG_BACKGROUND_EXPAND  0x0100
                          
#define PNG_16_TO_8            0x0400
#define PNG_RGBA               0x0800
#define PNG_EXPAND             0x1000
#define PNG_GAMMA              0x2000
#define PNG_GRAY_TO_RGB        0x4000
#define PNG_FILLER             0x8000L
#define PNG_PACKSWAP          0x10000L
#define PNG_SWAP_ALPHA        0x20000L
#define PNG_STRIP_ALPHA       0x40000L
#define PNG_INVERT_ALPHA      0x80000L
#define PNG_USER_TRANSFORM   0x100000L
#define PNG_RGB_TO_GRAY_ERR  0x200000L
#define PNG_RGB_TO_GRAY_WARN 0x400000L
#define PNG_RGB_TO_GRAY      0x600000L  /* two bits, RGB_TO_GRAY_ERR|WARN */
                       
#define PNG_ADD_ALPHA       0x1000000L  /* Added to libpng-1.2.7 */
#define PNG_EXPAND_tRNS     0x2000000L  /* Added to libpng-1.2.9 */
                       
                       
                       
                       
                       


#define PNG_STRUCT_PNG   0x0001
#define PNG_STRUCT_INFO  0x0002


#define PNG_WEIGHT_SHIFT 8
#define PNG_WEIGHT_FACTOR (1<<(PNG_WEIGHT_SHIFT))
#define PNG_COST_SHIFT 3
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
#define PNG_FLAG_FREE_PLTE                0x1000
#define PNG_FLAG_FREE_TRNS                0x2000
#define PNG_FLAG_FREE_HIST                0x4000
#define PNG_FLAG_KEEP_UNKNOWN_CHUNKS      0x8000L
#define PNG_FLAG_KEEP_UNSAFE_CHUNKS       0x10000L
#define PNG_FLAG_LIBRARY_MISMATCH         0x20000L
#define PNG_FLAG_STRIP_ERROR_NUMBERS      0x40000L
#define PNG_FLAG_STRIP_ERROR_TEXT         0x80000L
#define PNG_FLAG_MALLOC_NULL_MEM_OK       0x100000L
#define PNG_FLAG_ADD_ALPHA                0x200000L  /* Added to libpng-1.2.8 */
#define PNG_FLAG_STRIP_ALPHA              0x400000L  /* Added to libpng-1.2.8 */
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  

#define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)

#define PNG_FLAG_CRC_CRITICAL_MASK  (PNG_FLAG_CRC_CRITICAL_USE | \
                                     PNG_FLAG_CRC_CRITICAL_IGNORE)

#define PNG_FLAG_CRC_MASK           (PNG_FLAG_CRC_ANCILLARY_MASK | \
                                     PNG_FLAG_CRC_CRITICAL_MASK)



#define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))


#define PNG_ROWBYTES(pixel_bits, width) \
    ((pixel_bits) >= 8 ? \
    ((width) * (((png_uint_32)(pixel_bits)) >> 3)) : \
    (( ((width) * ((png_uint_32)(pixel_bits))) + 7) >> 3) )






#define PNG_OUT_OF_RANGE(value, ideal, delta) \
        ( (value) < (ideal)-(delta) || (value) > (ideal)+(delta) )


#if !defined(PNG_NO_EXTERN) || defined(PNG_ALWAYS_EXTERN)

#ifdef PNG_USE_GLOBAL_ARRAYS
   PNG_EXPORT_VAR (PNG_CONST png_byte FARDATA) png_sig[8];
#else
#endif
#endif 





#define PNG_IHDR png_byte png_IHDR[5] = { 73,  72,  68,  82, '\0'}
#define PNG_IDAT png_byte png_IDAT[5] = { 73,  68,  65,  84, '\0'}
#define PNG_IEND png_byte png_IEND[5] = { 73,  69,  78,  68, '\0'}
#define PNG_PLTE png_byte png_PLTE[5] = { 80,  76,  84,  69, '\0'}
#define PNG_bKGD png_byte png_bKGD[5] = { 98,  75,  71,  68, '\0'}
#define PNG_cHRM png_byte png_cHRM[5] = { 99,  72,  82,  77, '\0'}
#define PNG_gAMA png_byte png_gAMA[5] = {103,  65,  77,  65, '\0'}
#define PNG_hIST png_byte png_hIST[5] = {104,  73,  83,  84, '\0'}
#define PNG_iCCP png_byte png_iCCP[5] = {105,  67,  67,  80, '\0'}
#define PNG_iTXt png_byte png_iTXt[5] = {105,  84,  88, 116, '\0'}
#define PNG_oFFs png_byte png_oFFs[5] = {111,  70,  70, 115, '\0'}
#define PNG_pCAL png_byte png_pCAL[5] = {112,  67,  65,  76, '\0'}
#define PNG_sCAL png_byte png_sCAL[5] = {115,  67,  65,  76, '\0'}
#define PNG_pHYs png_byte png_pHYs[5] = {112,  72,  89, 115, '\0'}
#define PNG_sBIT png_byte png_sBIT[5] = {115,  66,  73,  84, '\0'}
#define PNG_sPLT png_byte png_sPLT[5] = {115,  80,  76,  84, '\0'}
#define PNG_sRGB png_byte png_sRGB[5] = {115,  82,  71,  66, '\0'}
#define PNG_tEXt png_byte png_tEXt[5] = {116,  69,  88, 116, '\0'}
#define PNG_tIME png_byte png_tIME[5] = {116,  73,  77,  69, '\0'}
#define PNG_tRNS png_byte png_tRNS[5] = {116,  82,  78,  83, '\0'}
#define PNG_zTXt png_byte png_zTXt[5] = {122,  84,  88, 116, '\0'}
#ifdef PNG_APNG_SUPPORTED
#define PNG_acTL png_byte png_acTL[5] = { 97,  99,  84,  76, '\0'}
#define PNG_fcTL png_byte png_fcTL[5] = {102,  99,  84,  76, '\0'}
#define PNG_fdAT png_byte png_fdAT[5] = {102, 100,  65,  84, '\0'}
#endif

#ifdef PNG_USE_GLOBAL_ARRAYS
PNG_EXPORT_VAR (png_byte FARDATA) png_IHDR[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_IDAT[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_IEND[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_PLTE[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_bKGD[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_cHRM[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_gAMA[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_hIST[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_iCCP[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_iTXt[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_oFFs[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_pCAL[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_sCAL[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_pHYs[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_sBIT[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_sPLT[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_sRGB[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_tEXt[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_tIME[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_tRNS[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_zTXt[5];
#ifdef PNG_APNG_SUPPORTED
PNG_EXPORT_VAR (png_byte FARDATA) png_acTL[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_fcTL[5];
PNG_EXPORT_VAR (png_byte FARDATA) png_fdAT[5];
#endif
#endif 

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)



extern PNG_EXPORT(void,png_read_init) PNGARG((png_structp png_ptr));
#undef png_read_init
#define png_read_init(png_ptr) png_read_init_3(&png_ptr, \
    PNG_LIBPNG_VER_STRING,  png_sizeof(png_struct));
#endif

extern PNG_EXPORT(void,png_read_init_3) PNGARG((png_structpp ptr_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size));
#if defined(PNG_1_0_X) || defined (PNG_1_2_X)
extern PNG_EXPORT(void,png_read_init_2) PNGARG((png_structp png_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size, png_size_t
    png_info_size));
#endif

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)



extern PNG_EXPORT(void,png_write_init) PNGARG((png_structp png_ptr));
#undef png_write_init
#define png_write_init(png_ptr) png_write_init_3(&png_ptr, \
    PNG_LIBPNG_VER_STRING, png_sizeof(png_struct));
#endif

extern PNG_EXPORT(void,png_write_init_3) PNGARG((png_structpp ptr_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size));
extern PNG_EXPORT(void,png_write_init_2) PNGARG((png_structp png_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size, png_size_t
    png_info_size));


PNG_EXTERN png_voidp png_create_struct PNGARG((int type));


PNG_EXTERN void png_destroy_struct PNGARG((png_voidp struct_ptr));

PNG_EXTERN png_voidp png_create_struct_2 PNGARG((int type, png_malloc_ptr
  malloc_fn, png_voidp mem_ptr));
PNG_EXTERN void png_destroy_struct_2 PNGARG((png_voidp struct_ptr,
   png_free_ptr free_fn, png_voidp mem_ptr));


PNG_EXTERN void png_info_destroy PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifndef PNG_1_0_X

PNG_EXTERN voidpf png_zalloc PNGARG((voidpf png_ptr, uInt items, uInt size));


PNG_EXTERN void png_zfree PNGARG((voidpf png_ptr, voidpf ptr));

#ifdef PNG_SIZE_T

   PNG_EXTERN png_size_t PNGAPI png_convert_size PNGARG((size_t size));
#endif





PNG_EXTERN void PNGAPI png_default_read_data PNGARG((png_structp png_ptr,
   png_bytep data, png_size_t length));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void PNGAPI png_push_fill_buffer PNGARG((png_structp png_ptr,
   png_bytep buffer, png_size_t length));
#endif

PNG_EXTERN void PNGAPI png_default_write_data PNGARG((png_structp png_ptr,
   png_bytep data, png_size_t length));

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
#if !defined(PNG_NO_STDIO)
PNG_EXTERN void PNGAPI png_default_flush PNGARG((png_structp png_ptr));
#endif
#endif
#else 
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void png_push_fill_buffer PNGARG((png_structp png_ptr,
   png_bytep buffer, png_size_t length));
#endif
#endif 


PNG_EXTERN void png_reset_crc PNGARG((png_structp png_ptr));


PNG_EXTERN void png_write_data PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));


PNG_EXTERN void png_read_data PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));


PNG_EXTERN void png_crc_read PNGARG((png_structp png_ptr, png_bytep buf,
   png_size_t length));


#if defined(PNG_zTXt_SUPPORTED) || defined(PNG_iTXt_SUPPORTED) || \
    defined(PNG_iCCP_SUPPORTED) || defined(PNG_sPLT_SUPPORTED)
PNG_EXTERN void png_decompress_chunk PNGARG((png_structp png_ptr,
   int comp_type, png_size_t chunklength,
   png_size_t prefix_length, png_size_t *data_length));
#endif


PNG_EXTERN int png_crc_finish PNGARG((png_structp png_ptr, png_uint_32 skip));


PNG_EXTERN int png_crc_error PNGARG((png_structp png_ptr));





PNG_EXTERN void png_calculate_crc PNGARG((png_structp png_ptr, png_bytep ptr,
   png_size_t length));

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
PNG_EXTERN void png_flush PNGARG((png_structp png_ptr));
#endif


PNG_EXTERN void png_write_sig PNGARG((png_structp png_ptr));






PNG_EXTERN void png_write_IHDR PNGARG((png_structp png_ptr, png_uint_32 width,
   png_uint_32 height,
   int bit_depth, int color_type, int compression_method, int filter_method,
   int interlace_method));

PNG_EXTERN void png_write_PLTE PNGARG((png_structp png_ptr, png_colorp palette,
   png_uint_32 num_pal));

PNG_EXTERN void png_write_IDAT PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

PNG_EXTERN void png_write_IEND PNGARG((png_structp png_ptr));

#if defined(PNG_WRITE_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_write_gAMA PNGARG((png_structp png_ptr, double file_gamma));
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXTERN void png_write_gAMA_fixed PNGARG((png_structp png_ptr, png_fixed_point
    file_gamma));
#endif
#endif

#if defined(PNG_WRITE_sBIT_SUPPORTED)
PNG_EXTERN void png_write_sBIT PNGARG((png_structp png_ptr, png_color_8p sbit,
   int color_type));
#endif

#if defined(PNG_WRITE_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_write_cHRM PNGARG((png_structp png_ptr,
   double white_x, double white_y,
   double red_x, double red_y, double green_x, double green_y,
   double blue_x, double blue_y));
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXTERN void png_write_cHRM_fixed PNGARG((png_structp png_ptr,
   png_fixed_point int_white_x, png_fixed_point int_white_y,
   png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
   int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
   png_fixed_point int_blue_y));
#endif
#endif

#if defined(PNG_WRITE_sRGB_SUPPORTED)
PNG_EXTERN void png_write_sRGB PNGARG((png_structp png_ptr,
   int intent));
#endif

#if defined(PNG_WRITE_iCCP_SUPPORTED)
PNG_EXTERN void png_write_iCCP PNGARG((png_structp png_ptr,
   png_charp name, int compression_type,
   png_charp profile, int proflen));
   
#endif

#if defined(PNG_WRITE_sPLT_SUPPORTED)
PNG_EXTERN void png_write_sPLT PNGARG((png_structp png_ptr,
   png_sPLT_tp palette));
#endif

#if defined(PNG_WRITE_tRNS_SUPPORTED)
PNG_EXTERN void png_write_tRNS PNGARG((png_structp png_ptr, png_bytep trans,
   png_color_16p values, int number, int color_type));
#endif

#if defined(PNG_WRITE_bKGD_SUPPORTED)
PNG_EXTERN void png_write_bKGD PNGARG((png_structp png_ptr,
   png_color_16p values, int color_type));
#endif

#if defined(PNG_WRITE_hIST_SUPPORTED)
PNG_EXTERN void png_write_hIST PNGARG((png_structp png_ptr, png_uint_16p hist,
   int num_hist));
#endif

#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED) || \
    defined(PNG_WRITE_iCCP_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
PNG_EXTERN png_size_t png_check_keyword PNGARG((png_structp png_ptr,
   png_charp key, png_charpp new_key));
#endif

#if defined(PNG_WRITE_tEXt_SUPPORTED)
PNG_EXTERN void png_write_tEXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_size_t text_len));
#endif

#if defined(PNG_WRITE_zTXt_SUPPORTED)
PNG_EXTERN void png_write_zTXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_size_t text_len, int compression));
#endif

#if defined(PNG_WRITE_iTXt_SUPPORTED)
PNG_EXTERN void png_write_iTXt PNGARG((png_structp png_ptr,
   int compression, png_charp key, png_charp lang, png_charp lang_key,
   png_charp text));
#endif

#if defined(PNG_TEXT_SUPPORTED)  
PNG_EXTERN int png_set_text_2 PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp text_ptr, int num_text));
#endif

#if defined(PNG_WRITE_oFFs_SUPPORTED)
PNG_EXTERN void png_write_oFFs PNGARG((png_structp png_ptr,
   png_int_32 x_offset, png_int_32 y_offset, int unit_type));
#endif

#if defined(PNG_WRITE_pCAL_SUPPORTED)
PNG_EXTERN void png_write_pCAL PNGARG((png_structp png_ptr, png_charp purpose,
   png_int_32 X0, png_int_32 X1, int type, int nparams,
   png_charp units, png_charpp params));
#endif

#if defined(PNG_WRITE_pHYs_SUPPORTED)
PNG_EXTERN void png_write_pHYs PNGARG((png_structp png_ptr,
   png_uint_32 x_pixels_per_unit, png_uint_32 y_pixels_per_unit,
   int unit_type));
#endif

#if defined(PNG_WRITE_tIME_SUPPORTED)
PNG_EXTERN void png_write_tIME PNGARG((png_structp png_ptr,
   png_timep mod_time));
#endif

#if defined(PNG_WRITE_sCAL_SUPPORTED)
#if defined(PNG_FLOATING_POINT_SUPPORTED) && !defined(PNG_NO_STDIO)
PNG_EXTERN void png_write_sCAL PNGARG((png_structp png_ptr,
   int unit, double width, double height));
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXTERN void png_write_sCAL_s PNGARG((png_structp png_ptr,
   int unit, png_charp width, png_charp height));
#endif
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
#endif


PNG_EXTERN void png_write_finish_row PNGARG((png_structp png_ptr));


PNG_EXTERN void png_write_start_row PNGARG((png_structp png_ptr));

#if defined(PNG_READ_GAMMA_SUPPORTED)
PNG_EXTERN void png_build_gamma_table PNGARG((png_structp png_ptr));
#endif


PNG_EXTERN void png_combine_row PNGARG((png_structp png_ptr, png_bytep row,
   int mask));

#if defined(PNG_READ_INTERLACING_SUPPORTED)





PNG_EXTERN void png_do_read_interlace PNGARG((png_structp png_ptr));
#endif



#if defined(PNG_WRITE_INTERLACING_SUPPORTED)

PNG_EXTERN void png_do_write_interlace PNGARG((png_row_infop row_info,
   png_bytep row, int pass));
#endif


PNG_EXTERN void png_read_filter_row PNGARG((png_structp png_ptr,
   png_row_infop row_info, png_bytep row, png_bytep prev_row, int filter));


PNG_EXTERN void png_write_find_filter PNGARG((png_structp png_ptr,
   png_row_infop row_info));


PNG_EXTERN void png_write_filtered_row PNGARG((png_structp png_ptr,
   png_bytep filtered_row));

PNG_EXTERN void png_read_finish_row PNGARG((png_structp png_ptr));


PNG_EXTERN void png_read_start_row PNGARG((png_structp png_ptr));

PNG_EXTERN void png_read_transform_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifdef PNG_READ_APNG_SUPPORTED

PNG_EXTERN void png_read_reset PNGARG((png_structp png_ptr));
PNG_EXTERN void png_read_reinit PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_progressive_read_reset PNGARG((png_structp png_ptr));
#endif
#ifdef PNG_WRITE_APNG_SUPPORTED

PNG_EXTERN void png_write_reset PNGARG((png_structp png_ptr));
PNG_EXTERN void png_write_reinit PNGARG((png_structp png_ptr, 
   png_infop info_ptr, png_uint_32 width, png_uint_32 height));
#endif


#if defined(PNG_READ_FILLER_SUPPORTED)
PNG_EXTERN void png_do_read_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 filler, png_uint_32 flags));
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_read_swap_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_WRITE_SWAP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_write_swap_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_read_invert_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_write_invert_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED) || \
    defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_strip_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 flags));
#endif

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
PNG_EXTERN void png_do_swap PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || defined(PNG_WRITE_PACKSWAP_SUPPORTED)
PNG_EXTERN void png_do_packswap PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
PNG_EXTERN int png_do_rgb_to_gray PNGARG((png_structp png_ptr, png_row_infop
   row_info, png_bytep row));
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
PNG_EXTERN void png_do_gray_to_rgb PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
PNG_EXTERN void png_do_unpack PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
PNG_EXTERN void png_do_unshift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p sig_bits));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
PNG_EXTERN void png_do_invert PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
PNG_EXTERN void png_do_chop PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
PNG_EXTERN void png_do_dither PNGARG((png_row_infop row_info,
   png_bytep row, png_bytep palette_lookup, png_bytep dither_lookup));

#  if defined(PNG_CORRECT_PALETTE_SUPPORTED)
PNG_EXTERN void png_correct_palette PNGARG((png_structp png_ptr,
   png_colorp palette, int num_palette));
#  endif
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
PNG_EXTERN void png_do_bgr PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_WRITE_PACK_SUPPORTED)
PNG_EXTERN void png_do_pack PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 bit_depth));
#endif

#if defined(PNG_WRITE_SHIFT_SUPPORTED)
PNG_EXTERN void png_do_shift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p bit_depth));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
#if defined(PNG_READ_GAMMA_SUPPORTED)
PNG_EXTERN void png_do_background PNGARG((png_row_infop row_info, png_bytep row,
   png_color_16p trans_values, png_color_16p background,
   png_color_16p background_1,
   png_bytep gamma_table, png_bytep gamma_from_1, png_bytep gamma_to_1,
   png_uint_16pp gamma_16, png_uint_16pp gamma_16_from_1,
   png_uint_16pp gamma_16_to_1, int gamma_shift));
#else
PNG_EXTERN void png_do_background PNGARG((png_row_infop row_info, png_bytep row,
   png_color_16p trans_values, png_color_16p background));
#endif
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
PNG_EXTERN void png_do_gamma PNGARG((png_row_infop row_info, png_bytep row,
   png_bytep gamma_table, png_uint_16pp gamma_16_table,
   int gamma_shift));
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
PNG_EXTERN void png_do_expand_palette PNGARG((png_row_infop row_info,
   png_bytep row, png_colorp palette, png_bytep trans, int num_trans));
PNG_EXTERN void png_do_expand PNGARG((png_row_infop row_info,
   png_bytep row, png_color_16p trans_value));
#endif






PNG_EXTERN void png_handle_IHDR PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_PLTE PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_IEND PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

#if defined(PNG_READ_bKGD_SUPPORTED)
PNG_EXTERN void png_handle_bKGD PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
PNG_EXTERN void png_handle_cHRM PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_gAMA_SUPPORTED)
PNG_EXTERN void png_handle_gAMA PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
PNG_EXTERN void png_handle_hIST PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_iCCP_SUPPORTED)
extern void png_handle_iCCP PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif 

#if defined(PNG_READ_iTXt_SUPPORTED)
PNG_EXTERN void png_handle_iTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
PNG_EXTERN void png_handle_oFFs PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_pCAL_SUPPORTED)
PNG_EXTERN void png_handle_pCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
PNG_EXTERN void png_handle_pHYs PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
PNG_EXTERN void png_handle_sBIT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_sCAL_SUPPORTED)
PNG_EXTERN void png_handle_sCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_sPLT_SUPPORTED)
extern void png_handle_sPLT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif 

#if defined(PNG_READ_sRGB_SUPPORTED)
PNG_EXTERN void png_handle_sRGB PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
PNG_EXTERN void png_handle_tEXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
PNG_EXTERN void png_handle_tIME PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
PNG_EXTERN void png_handle_tRNS PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
PNG_EXTERN void png_handle_zTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#ifdef PNG_READ_APNG_SUPPORTED
PNG_EXTERN void png_handle_acTL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_fcTL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_have_info PNGARG((png_structp png_ptr, png_infop info_ptr));
PNG_EXTERN void png_handle_fdAT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_ensure_sequence_number PNGARG((png_structp png_ptr, 
   png_uint_32 length));
#endif

PNG_EXTERN void png_handle_unknown PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));

PNG_EXTERN void png_check_chunk_name PNGARG((png_structp png_ptr,
   png_bytep chunk_name));


PNG_EXTERN void png_do_read_transformations PNGARG((png_structp png_ptr));
PNG_EXTERN void png_do_write_transformations PNGARG((png_structp png_ptr));

PNG_EXTERN void png_init_read_transformations PNGARG((png_structp png_ptr));

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
#if defined(PNG_READ_tEXt_SUPPORTED)
PNG_EXTERN void png_push_handle_tEXt PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_tEXt PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
PNG_EXTERN void png_push_handle_zTXt PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_zTXt PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
PNG_EXTERN void png_push_handle_iTXt PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_iTXt PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif

#endif 

#ifdef PNG_MNG_FEATURES_SUPPORTED
PNG_EXTERN void png_do_read_intrapixel PNGARG((png_row_infop row_info,
   png_bytep row));
PNG_EXTERN void png_do_write_intrapixel PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
#if defined(PNG_MMX_CODE_SUPPORTED)
 
PNG_EXTERN void png_init_mmx_flags PNGARG((png_structp png_ptr));
#endif
#endif

#if defined(PNG_INCH_CONVERSIONS) && defined(PNG_FLOATING_POINT_SUPPORTED)
PNG_EXTERN png_uint_32 png_get_pixels_per_inch PNGARG((png_structp png_ptr,
png_infop info_ptr));

PNG_EXTERN png_uint_32 png_get_x_pixels_per_inch PNGARG((png_structp png_ptr,
png_infop info_ptr));

PNG_EXTERN png_uint_32 png_get_y_pixels_per_inch PNGARG((png_structp png_ptr,
png_infop info_ptr));

PNG_EXTERN float png_get_x_offset_inches PNGARG((png_structp png_ptr,
png_infop info_ptr));

PNG_EXTERN float png_get_y_offset_inches PNGARG((png_structp png_ptr,
png_infop info_ptr));

#if defined(PNG_pHYs_SUPPORTED)
PNG_EXTERN png_uint_32 png_get_pHYs_dpi PNGARG((png_structp png_ptr,
png_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type));
#endif 
#endif  


PNG_EXTERN png_uint_32 png_read_chunk_header PNGARG((png_structp png_ptr));


#if defined(PNG_cHRM_SUPPORTED)
PNG_EXTERN int png_check_cHRM_fixed  PNGARG((png_structp png_ptr,
   png_fixed_point int_white_x, png_fixed_point int_white_y,
   png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
   int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
   png_fixed_point int_blue_y));
#endif

#if defined(PNG_cHRM_SUPPORTED)
#if !defined(PNG_NO_CHECK_cHRM)

PNG_EXTERN void png_64bit_product (long v1, long v2, unsigned long *hi_product,
   unsigned long *lo_product);
#endif
#endif



#endif 

#ifdef __cplusplus
}
#endif

#endif

#endif
