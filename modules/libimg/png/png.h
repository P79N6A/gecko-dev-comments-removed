


































































































































































































































































































































































#ifndef PNG_H
#define PNG_H








#define PNG_LIBPNG_VER_STRING "1.4.7"
#define PNG_HEADER_VERSION_STRING \
   " libpng version 1.4.7 - April 10, 2011\n"

#define PNG_LIBPNG_VER_SONUM   14
#define PNG_LIBPNG_VER_DLLNUM  14


#define PNG_LIBPNG_VER_MAJOR   1
#define PNG_LIBPNG_VER_MINOR   4
#define PNG_LIBPNG_VER_RELEASE 7




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

#define PNG_LIBPNG_BUILD_BASE_TYPE PNG_LIBPNG_BUILD_BETA







#define PNG_LIBPNG_VER 10407 /* 1.4.7 */

#ifndef PNG_VERSION_INFO_ONLY

#include "zlib.h"
#endif


#include "pngconf.h"















#ifdef PNG_USER_PRIVATEBUILD
#  define PNG_LIBPNG_BUILD_TYPE \
          (PNG_LIBPNG_BUILD_BASE_TYPE | PNG_LIBPNG_BUILD_PRIVATE)
#else
#  ifdef PNG_LIBPNG_SPECIALBUILD
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








#if !defined(PNG_NO_EXTERN) || defined(PNG_ALWAYS_EXTERN)



#define png_libpng_ver png_get_header_ver(NULL)

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





typedef struct png_unknown_chunk_t
{
    png_byte name[5];
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
   
   png_uint_32 width PNG_DEPSTRUCT;  
   png_uint_32 height PNG_DEPSTRUCT; 
   png_uint_32 valid PNG_DEPSTRUCT;  

   png_size_t rowbytes PNG_DEPSTRUCT; 

   png_colorp palette PNG_DEPSTRUCT;      

   png_uint_16 num_palette PNG_DEPSTRUCT; 

   png_uint_16 num_trans PNG_DEPSTRUCT;   

   png_byte bit_depth PNG_DEPSTRUCT;      

   png_byte color_type PNG_DEPSTRUCT;     

   
   png_byte compression_type PNG_DEPSTRUCT; 

   png_byte filter_type PNG_DEPSTRUCT;    

   png_byte interlace_type PNG_DEPSTRUCT; 


   
   png_byte channels PNG_DEPSTRUCT;       

   png_byte pixel_depth PNG_DEPSTRUCT;    
   png_byte spare_byte PNG_DEPSTRUCT;     

   png_byte signature[8] PNG_DEPSTRUCT;   


   





#if defined(PNG_gAMA_SUPPORTED) && defined(PNG_FLOATING_POINT_SUPPORTED)
   



   float gamma PNG_DEPSTRUCT; 

#endif

#ifdef PNG_sRGB_SUPPORTED
    
    
   png_byte srgb_intent PNG_DEPSTRUCT; 

#endif

#ifdef PNG_TEXT_SUPPORTED
   







   int num_text PNG_DEPSTRUCT; 
   int max_text PNG_DEPSTRUCT; 
   png_textp text PNG_DEPSTRUCT; 
#endif 

#ifdef PNG_tIME_SUPPORTED
   


   png_time mod_time PNG_DEPSTRUCT;
#endif

#ifdef PNG_sBIT_SUPPORTED
   





   png_color_8 sig_bit PNG_DEPSTRUCT; 
#endif

#if defined(PNG_tRNS_SUPPORTED) || defined(PNG_READ_EXPAND_SUPPORTED) || \
defined(PNG_READ_BACKGROUND_SUPPORTED)
   








   png_bytep trans_alpha PNG_DEPSTRUCT;    

   png_color_16 trans_color PNG_DEPSTRUCT; 

#endif

#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   





   png_color_16 background PNG_DEPSTRUCT;
#endif

#ifdef PNG_oFFs_SUPPORTED
   




   png_int_32 x_offset PNG_DEPSTRUCT; 
   png_int_32 y_offset PNG_DEPSTRUCT; 
   png_byte offset_unit_type PNG_DEPSTRUCT; 
#endif

#ifdef PNG_pHYs_SUPPORTED
   



   png_uint_32 x_pixels_per_unit PNG_DEPSTRUCT; 
   png_uint_32 y_pixels_per_unit PNG_DEPSTRUCT; 
   png_byte phys_unit_type PNG_DEPSTRUCT; 

#endif

#ifdef PNG_hIST_SUPPORTED
   





   png_uint_16p hist PNG_DEPSTRUCT;
#endif

#ifdef PNG_cHRM_SUPPORTED
   





#ifdef PNG_FLOATING_POINT_SUPPORTED
   float x_white PNG_DEPSTRUCT;
   float y_white PNG_DEPSTRUCT;
   float x_red PNG_DEPSTRUCT;
   float y_red PNG_DEPSTRUCT;
   float x_green PNG_DEPSTRUCT;
   float y_green PNG_DEPSTRUCT;
   float x_blue PNG_DEPSTRUCT;
   float y_blue PNG_DEPSTRUCT;
#endif
#endif

#ifdef PNG_pCAL_SUPPORTED
   










   png_charp pcal_purpose PNG_DEPSTRUCT;  
   png_int_32 pcal_X0 PNG_DEPSTRUCT;      
   png_int_32 pcal_X1 PNG_DEPSTRUCT;      
   png_charp pcal_units PNG_DEPSTRUCT;    

   png_charpp pcal_params PNG_DEPSTRUCT;  

   png_byte pcal_type PNG_DEPSTRUCT;      

   png_byte pcal_nparams PNG_DEPSTRUCT;   

#endif


   png_uint_32 free_me PNG_DEPSTRUCT;     


#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED) || \
 defined(PNG_HANDLE_AS_UNKNOWN_SUPPORTED)
   
   png_unknown_chunkp unknown_chunks PNG_DEPSTRUCT;
   png_size_t unknown_chunks_num PNG_DEPSTRUCT;
#endif

#ifdef PNG_iCCP_SUPPORTED
   
   png_charp iccp_name PNG_DEPSTRUCT;     
   png_charp iccp_profile PNG_DEPSTRUCT;  

                            
   png_uint_32 iccp_proflen PNG_DEPSTRUCT;  
   png_byte iccp_compression PNG_DEPSTRUCT; 
#endif

#ifdef PNG_sPLT_SUPPORTED
   
   png_sPLT_tp splt_palettes PNG_DEPSTRUCT;
   png_uint_32 splt_palettes_num PNG_DEPSTRUCT;
#endif

#ifdef PNG_sCAL_SUPPORTED
   






   png_byte scal_unit PNG_DEPSTRUCT;         
#ifdef PNG_FLOATING_POINT_SUPPORTED
   double scal_pixel_width PNG_DEPSTRUCT;    
   double scal_pixel_height PNG_DEPSTRUCT;   
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   png_charp scal_s_width PNG_DEPSTRUCT;     
   png_charp scal_s_height PNG_DEPSTRUCT;    
#endif
#endif

#ifdef PNG_INFO_IMAGE_SUPPORTED
   

   
   png_bytepp row_pointers PNG_DEPSTRUCT;        
#endif

#if defined(PNG_FIXED_POINT_SUPPORTED) && defined(PNG_gAMA_SUPPORTED)
   png_fixed_point int_gamma PNG_DEPSTRUCT; 

#endif

#if defined(PNG_cHRM_SUPPORTED) && defined(PNG_FIXED_POINT_SUPPORTED)
   png_fixed_point int_x_white PNG_DEPSTRUCT;
   png_fixed_point int_y_white PNG_DEPSTRUCT;
   png_fixed_point int_x_red PNG_DEPSTRUCT;
   png_fixed_point int_y_red PNG_DEPSTRUCT;
   png_fixed_point int_x_green PNG_DEPSTRUCT;
   png_fixed_point int_y_green PNG_DEPSTRUCT;
   png_fixed_point int_x_blue PNG_DEPSTRUCT;
   png_fixed_point int_y_blue PNG_DEPSTRUCT;
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
typedef PNG_CONST png_info FAR * png_const_infop;
typedef png_info FAR * FAR * png_infopp;


#define PNG_UINT_31_MAX ((png_uint_32)0x7fffffffL)
#define PNG_UINT_32_MAX ((png_uint_32)(-1))
#define PNG_SIZE_MAX ((png_size_t)(-1))



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
   png_size_t rowbytes; 
   png_byte color_type; 
   png_byte bit_depth; 
   png_byte channels; 
   png_byte pixel_depth; 
} png_row_info;

typedef png_row_info FAR * png_row_infop;
typedef png_row_info FAR * FAR * png_row_infopp;







typedef struct png_struct_def png_struct;
typedef png_struct FAR * png_structp;
typedef PNG_CONST png_struct FAR * png_const_structp;

typedef void (PNGAPI *png_error_ptr) PNGARG((png_structp, png_const_charp));
typedef void (PNGAPI *png_rw_ptr) PNGARG((png_structp, png_bytep, png_size_t));
typedef void (PNGAPI *png_flush_ptr) PNGARG((png_structp));
typedef void (PNGAPI *png_read_status_ptr) PNGARG((png_structp, png_uint_32,
   int));
typedef void (PNGAPI *png_write_status_ptr) PNGARG((png_structp, png_uint_32,
   int));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
typedef void (PNGAPI *png_progressive_info_ptr) PNGARG((png_structp,
   png_infop));
typedef void (PNGAPI *png_progressive_end_ptr) PNGARG((png_structp, png_infop));
typedef void (PNGAPI *png_progressive_row_ptr) PNGARG((png_structp, png_bytep,
   png_uint_32, int));

#ifdef PNG_APNG_SUPPORTED
typedef void (PNGAPI *png_progressive_frame_ptr) PNGARG((png_structp,
   png_uint_32));
#endif
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
typedef void (PNGAPI *png_user_transform_ptr) PNGARG((png_structp,
   png_row_infop, png_bytep));
#endif

#ifdef PNG_USER_CHUNKS_SUPPORTED
typedef int (PNGAPI *png_user_chunk_ptr) PNGARG((png_structp,
   png_unknown_chunkp));
#endif
#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
typedef void (PNGAPI *png_unknown_chunk_ptr) PNGARG((png_structp));
#endif
#ifdef PNG_SETJMP_SUPPORTED




typedef void (PNGAPI *png_longjmp_ptr) PNGARG((jmp_buf, int));
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
#define PNG_TRANSFORM_STRIP_FILLER   0x0800    /* write only */

#define PNG_TRANSFORM_STRIP_FILLER_BEFORE PNG_TRANSFORM_STRIP_FILLER
#define PNG_TRANSFORM_STRIP_FILLER_AFTER 0x1000 /* write only */

#define PNG_TRANSFORM_GRAY_TO_RGB   0x2000      /* read only */


#define PNG_FLAG_MNG_EMPTY_PLTE     0x01
#define PNG_FLAG_MNG_FILTER_64      0x04
#define PNG_ALL_MNG_FEATURES        0x05

typedef png_voidp (*png_malloc_ptr) PNGARG((png_structp, png_alloc_size_t));
typedef void (*png_free_ptr) PNGARG((png_structp, png_voidp));








struct png_struct_def
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf jmpbuf PNG_DEPSTRUCT;            
   png_longjmp_ptr longjmp_fn PNG_DEPSTRUCT;

#endif
   png_error_ptr error_fn PNG_DEPSTRUCT;    

   png_error_ptr warning_fn PNG_DEPSTRUCT;  

   png_voidp error_ptr PNG_DEPSTRUCT;       

   png_rw_ptr write_data_fn PNG_DEPSTRUCT;  

   png_rw_ptr read_data_fn PNG_DEPSTRUCT;   

   png_voidp io_ptr PNG_DEPSTRUCT;          


#ifdef PNG_READ_USER_TRANSFORM_SUPPORTED
   png_user_transform_ptr read_user_transform_fn PNG_DEPSTRUCT; 

#endif

#ifdef PNG_WRITE_USER_TRANSFORM_SUPPORTED
   png_user_transform_ptr write_user_transform_fn PNG_DEPSTRUCT; 

#endif


#ifdef PNG_USER_TRANSFORM_PTR_SUPPORTED
#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
   png_voidp user_transform_ptr PNG_DEPSTRUCT; 

   png_byte user_transform_depth PNG_DEPSTRUCT;    

   png_byte user_transform_channels PNG_DEPSTRUCT; 

#endif
#endif

   png_uint_32 mode PNG_DEPSTRUCT;          

   png_uint_32 flags PNG_DEPSTRUCT;         

   png_uint_32 transformations PNG_DEPSTRUCT; 


   z_stream zstream PNG_DEPSTRUCT;          

   png_bytep zbuf PNG_DEPSTRUCT;            
   png_size_t zbuf_size PNG_DEPSTRUCT;      
   int zlib_level PNG_DEPSTRUCT;            
   int zlib_method PNG_DEPSTRUCT;           
   int zlib_window_bits PNG_DEPSTRUCT;      

   int zlib_mem_level PNG_DEPSTRUCT;        

   int zlib_strategy PNG_DEPSTRUCT;         


   png_uint_32 width PNG_DEPSTRUCT;         
   png_uint_32 height PNG_DEPSTRUCT;        
   png_uint_32 num_rows PNG_DEPSTRUCT;      
   png_uint_32 usr_width PNG_DEPSTRUCT;     
   png_size_t rowbytes PNG_DEPSTRUCT;       
#if 0 
   png_size_t irowbytes PNG_DEPSTRUCT;
#endif

#ifdef PNG_USER_LIMITS_SUPPORTED
   




   png_alloc_size_t user_chunk_malloc_max PNG_DEPSTRUCT;
#endif
   png_uint_32 iwidth PNG_DEPSTRUCT;        

   png_uint_32 row_number PNG_DEPSTRUCT;    
   png_bytep prev_row PNG_DEPSTRUCT;        

   png_bytep row_buf PNG_DEPSTRUCT;         

   png_bytep sub_row PNG_DEPSTRUCT;         

   png_bytep up_row PNG_DEPSTRUCT;          

   png_bytep avg_row PNG_DEPSTRUCT;         

   png_bytep paeth_row PNG_DEPSTRUCT;       

   png_row_info row_info PNG_DEPSTRUCT;     


   png_uint_32 idat_size PNG_DEPSTRUCT;     
   png_uint_32 crc PNG_DEPSTRUCT;           
   png_colorp palette PNG_DEPSTRUCT;        
   png_uint_16 num_palette PNG_DEPSTRUCT;   

   png_uint_16 num_trans PNG_DEPSTRUCT;     
   png_byte chunk_name[5] PNG_DEPSTRUCT;    

   png_byte compression PNG_DEPSTRUCT;      

   png_byte filter PNG_DEPSTRUCT;           
   png_byte interlaced PNG_DEPSTRUCT;       

   png_byte pass PNG_DEPSTRUCT;             
   png_byte do_filter PNG_DEPSTRUCT;        

   png_byte color_type PNG_DEPSTRUCT;       
   png_byte bit_depth PNG_DEPSTRUCT;        
   png_byte usr_bit_depth PNG_DEPSTRUCT;    
   png_byte pixel_depth PNG_DEPSTRUCT;      
   png_byte channels PNG_DEPSTRUCT;         
   png_byte usr_channels PNG_DEPSTRUCT;     
   png_byte sig_bytes PNG_DEPSTRUCT;        


#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
   png_uint_16 filler PNG_DEPSTRUCT;           

#endif

#ifdef PNG_bKGD_SUPPORTED
   png_byte background_gamma_type PNG_DEPSTRUCT;
#  ifdef PNG_FLOATING_POINT_SUPPORTED
   float background_gamma PNG_DEPSTRUCT;
#  endif
   png_color_16 background PNG_DEPSTRUCT;   

#ifdef PNG_READ_GAMMA_SUPPORTED
   png_color_16 background_1 PNG_DEPSTRUCT; 

#endif
#endif 

#ifdef PNG_WRITE_FLUSH_SUPPORTED
   png_flush_ptr output_flush_fn PNG_DEPSTRUCT; 

   png_uint_32 flush_dist PNG_DEPSTRUCT;    

   png_uint_32 flush_rows PNG_DEPSTRUCT;    

#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   int gamma_shift PNG_DEPSTRUCT;      

#ifdef PNG_FLOATING_POINT_SUPPORTED
   float gamma PNG_DEPSTRUCT;          
   float screen_gamma PNG_DEPSTRUCT;   

#endif
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep gamma_table PNG_DEPSTRUCT;     

   png_bytep gamma_from_1 PNG_DEPSTRUCT;    
   png_bytep gamma_to_1 PNG_DEPSTRUCT;      
   png_uint_16pp gamma_16_table PNG_DEPSTRUCT; 

   png_uint_16pp gamma_16_from_1 PNG_DEPSTRUCT; 

   png_uint_16pp gamma_16_to_1 PNG_DEPSTRUCT; 
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_sBIT_SUPPORTED)
   png_color_8 sig_bit PNG_DEPSTRUCT;       

#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
   png_color_8 shift PNG_DEPSTRUCT;         

#endif

#if defined(PNG_tRNS_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) \
 || defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep trans_alpha PNG_DEPSTRUCT;           

   png_color_16 trans_color PNG_DEPSTRUCT;  

#endif

   png_read_status_ptr read_row_fn PNG_DEPSTRUCT;   

   png_write_status_ptr write_row_fn PNG_DEPSTRUCT; 

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_progressive_info_ptr info_fn PNG_DEPSTRUCT; 

   png_progressive_row_ptr row_fn PNG_DEPSTRUCT;   

   png_progressive_end_ptr end_fn PNG_DEPSTRUCT;   

   png_bytep save_buffer_ptr PNG_DEPSTRUCT;        

   png_bytep save_buffer PNG_DEPSTRUCT;            

   png_bytep current_buffer_ptr PNG_DEPSTRUCT;     

   png_bytep current_buffer PNG_DEPSTRUCT;         

   png_uint_32 push_length PNG_DEPSTRUCT;          

   png_uint_32 skip_length PNG_DEPSTRUCT;          

   png_size_t save_buffer_size PNG_DEPSTRUCT;      

   png_size_t save_buffer_max PNG_DEPSTRUCT;       

   png_size_t buffer_size PNG_DEPSTRUCT;           

   png_size_t current_buffer_size PNG_DEPSTRUCT;   

   int process_mode PNG_DEPSTRUCT;                 

   int cur_palette PNG_DEPSTRUCT;                  


#  ifdef PNG_TEXT_SUPPORTED
     png_size_t current_text_size PNG_DEPSTRUCT;   

     png_size_t current_text_left PNG_DEPSTRUCT;   

     png_charp current_text PNG_DEPSTRUCT;         

     png_charp current_text_ptr PNG_DEPSTRUCT;     

#  endif 

#endif 

#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)

   png_bytepp offset_table_ptr PNG_DEPSTRUCT;
   png_bytep offset_table PNG_DEPSTRUCT;
   png_uint_16 offset_table_number PNG_DEPSTRUCT;
   png_uint_16 offset_table_count PNG_DEPSTRUCT;
   png_uint_16 offset_table_count_free PNG_DEPSTRUCT;
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED
   png_bytep palette_lookup PNG_DEPSTRUCT; 
   png_bytep quantize_index PNG_DEPSTRUCT; 

#endif

#if defined(PNG_READ_QUANTIZE_SUPPORTED) || defined(PNG_hIST_SUPPORTED)
   png_uint_16p hist PNG_DEPSTRUCT;                
#endif

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   png_byte heuristic_method PNG_DEPSTRUCT;        

   png_byte num_prev_filters PNG_DEPSTRUCT;        

   png_bytep prev_filters PNG_DEPSTRUCT;           

   png_uint_16p filter_weights PNG_DEPSTRUCT;      

   png_uint_16p inv_filter_weights PNG_DEPSTRUCT;  

   png_uint_16p filter_costs PNG_DEPSTRUCT;        

   png_uint_16p inv_filter_costs PNG_DEPSTRUCT;    

#endif

#ifdef PNG_TIME_RFC1123_SUPPORTED
   png_charp time_buffer PNG_DEPSTRUCT; 
#endif



   png_uint_32 free_me PNG_DEPSTRUCT;    


#ifdef PNG_USER_CHUNKS_SUPPORTED
   png_voidp user_chunk_ptr PNG_DEPSTRUCT;
   png_user_chunk_ptr read_user_chunk_fn PNG_DEPSTRUCT; 

#endif

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   int num_chunk_list PNG_DEPSTRUCT;
   png_bytep chunk_list PNG_DEPSTRUCT;
#endif


#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
   png_byte rgb_to_gray_status PNG_DEPSTRUCT;
   
   png_uint_16 rgb_to_gray_red_coeff PNG_DEPSTRUCT;
   png_uint_16 rgb_to_gray_green_coeff PNG_DEPSTRUCT;
   png_uint_16 rgb_to_gray_blue_coeff PNG_DEPSTRUCT;
#endif


#if defined(PNG_MNG_FEATURES_SUPPORTED) || \
    defined(PNG_READ_EMPTY_PLTE_SUPPORTED) || \
    defined(PNG_WRITE_EMPTY_PLTE_SUPPORTED)

   png_uint_32 mng_features_permitted PNG_DEPSTRUCT;
#endif


#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_fixed_point int_gamma PNG_DEPSTRUCT;
#endif


#ifdef PNG_MNG_FEATURES_SUPPORTED
   png_byte filter_type PNG_DEPSTRUCT;
#endif




#ifdef PNG_USER_MEM_SUPPORTED
   png_voidp mem_ptr PNG_DEPSTRUCT;             

   png_malloc_ptr malloc_fn PNG_DEPSTRUCT;      

   png_free_ptr free_fn PNG_DEPSTRUCT;          

#endif


   png_bytep big_row_buf PNG_DEPSTRUCT;         


#ifdef PNG_READ_QUANTIZE_SUPPORTED

   png_bytep quantize_sort PNG_DEPSTRUCT;          
   png_bytep index_to_palette PNG_DEPSTRUCT;       


   png_bytep palette_to_index PNG_DEPSTRUCT;       


#endif


   png_byte compression_type PNG_DEPSTRUCT;

#ifdef PNG_USER_LIMITS_SUPPORTED
   png_uint_32 user_width_max PNG_DEPSTRUCT;
   png_uint_32 user_height_max PNG_DEPSTRUCT;
   


   png_uint_32 user_chunk_cache_max PNG_DEPSTRUCT;
#endif


#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
   
   png_unknown_chunk unknown_chunk PNG_DEPSTRUCT;
#endif


  png_uint_32 old_big_row_buf_size PNG_DEPSTRUCT;
  png_uint_32 old_prev_row_size PNG_DEPSTRUCT;


  png_charp chunkdata PNG_DEPSTRUCT;  

#ifdef PNG_IO_STATE_SUPPORTED

   png_uint_32 io_state PNG_DEPSTRUCT;
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
#endif 
};

#ifdef PNG_APNG_SUPPORTED

#define PNG_FIRST_FRAME_HIDDEN       0x0001


#define PNG_DISPOSE_OP_NONE        0x00
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#define PNG_DISPOSE_OP_PREVIOUS    0x02


#define PNG_BLEND_OP_SOURCE        0x00
#define PNG_BLEND_OP_OVER          0x01
#endif 




typedef png_structp version_1_4_7;

typedef png_struct FAR * FAR * png_structpp;








PNG_EXPORT(png_uint_32,png_access_version_number) PNGARG((void));




PNG_EXPORT(void,png_set_sig_bytes) PNGARG((png_structp png_ptr,
   int num_bytes));






PNG_EXPORT(int,png_sig_cmp) PNGARG((png_bytep sig, png_size_t start,
   png_size_t num_to_check));




#define png_check_sig(sig,n) !png_sig_cmp((sig), 0, (n))


PNG_EXPORT(png_structp,png_create_read_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn)) PNG_ALLOCATED;


PNG_EXPORT(png_structp,png_create_write_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn)) PNG_ALLOCATED;

PNG_EXPORT(png_size_t,png_get_compression_buffer_size)
   PNGARG((png_const_structp png_ptr));

PNG_EXPORT(void,png_set_compression_buffer_size)
   PNGARG((png_structp png_ptr, png_size_t size));




#ifdef PNG_SETJMP_SUPPORTED







PNG_EXPORT(jmp_buf*, png_set_longjmp_fn)
   PNGARG((png_structp png_ptr, png_longjmp_ptr longjmp_fn, size_t
       jmp_buf_size));
#  define png_jmpbuf(png_ptr) \
   (*png_set_longjmp_fn((png_ptr), longjmp, sizeof (jmp_buf)))
#else
#  define png_jmpbuf(png_ptr) \
   (LIBPNG_WAS_COMPILED_WITH__PNG_NO_SETJMP)
#endif

#ifdef PNG_READ_SUPPORTED

PNG_EXPORT(int,png_reset_zstream) PNGARG((png_structp png_ptr));
#endif


#ifdef PNG_USER_MEM_SUPPORTED
PNG_EXPORT(png_structp,png_create_read_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn)) PNG_ALLOCATED;
PNG_EXPORT(png_structp,png_create_write_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn)) PNG_ALLOCATED;
#endif


PNG_EXPORT(void,png_write_sig) PNGARG((png_structp png_ptr));


PNG_EXPORT(void,png_write_chunk) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_bytep data, png_size_t length));


PNG_EXPORT(void,png_write_chunk_start) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_uint_32 length));


PNG_EXPORT(void,png_write_chunk_data) PNGARG((png_structp png_ptr,
   png_bytep data, png_size_t length));


PNG_EXPORT(void,png_write_chunk_end) PNGARG((png_structp png_ptr));


PNG_EXPORT(png_infop,png_create_info_struct)
   PNGARG((png_structp png_ptr)) PNG_ALLOCATED;

PNG_EXPORT(void,png_info_init_3) PNGARG((png_infopp info_ptr,
    png_size_t png_info_struct_size));


PNG_EXPORT(void,png_write_info_before_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXPORT(void,png_write_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED

PNG_EXPORT(void,png_read_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif

#ifdef PNG_TIME_RFC1123_SUPPORTED
PNG_EXPORT(png_charp,png_convert_to_rfc1123)
   PNGARG((png_structp png_ptr, png_timep ptime));
#endif

#ifdef PNG_CONVERT_tIME_SUPPORTED

PNG_EXPORT(void,png_convert_from_struct_tm) PNGARG((png_timep ptime,
   struct tm FAR * ttime));


PNG_EXPORT(void,png_convert_from_time_t) PNGARG((png_timep ptime,
   time_t ttime));
#endif 

#ifdef PNG_READ_EXPAND_SUPPORTED

PNG_EXPORT(void,png_set_expand) PNGARG((png_structp png_ptr));
PNG_EXPORT(void,png_set_expand_gray_1_2_4_to_8) PNGARG((png_structp
  png_ptr));
PNG_EXPORT(void,png_set_palette_to_rgb) PNGARG((png_structp png_ptr));
PNG_EXPORT(void,png_set_tRNS_to_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)

PNG_EXPORT(void,png_set_bgr) PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED

PNG_EXPORT(void,png_set_gray_to_rgb) PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED

#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_rgb_to_gray) PNGARG((png_structp png_ptr,
   int error_action, double red, double green ));
#endif
PNG_EXPORT(void,png_set_rgb_to_gray_fixed) PNGARG((png_structp png_ptr,
   int error_action, png_fixed_point red, png_fixed_point green ));
PNG_EXPORT(png_byte,png_get_rgb_to_gray_status) PNGARG((png_const_structp
   png_ptr));
#endif

PNG_EXPORT(void,png_build_grayscale_palette) PNGARG((int bit_depth,
   png_colorp palette));

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
PNG_EXPORT(void,png_set_strip_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED) || \
    defined(PNG_WRITE_SWAP_ALPHA_SUPPORTED)
PNG_EXPORT(void,png_set_swap_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED) || \
    defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
PNG_EXPORT(void,png_set_invert_alpha) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)

PNG_EXPORT(void,png_set_filler) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));

#define PNG_FILLER_BEFORE 0
#define PNG_FILLER_AFTER 1

PNG_EXPORT(void,png_set_add_alpha) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));
#endif 

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)

PNG_EXPORT(void,png_set_swap) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_PACK_SUPPORTED) || defined(PNG_WRITE_PACK_SUPPORTED)

PNG_EXPORT(void,png_set_packing) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || \
    defined(PNG_WRITE_PACKSWAP_SUPPORTED)

PNG_EXPORT(void,png_set_packswap) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)

PNG_EXPORT(void,png_set_shift) PNGARG((png_structp png_ptr,
   png_color_8p true_bits));
#endif

#if defined(PNG_READ_INTERLACING_SUPPORTED) || \
    defined(PNG_WRITE_INTERLACING_SUPPORTED)

PNG_EXPORT(int,png_set_interlace_handling) PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)

PNG_EXPORT(void,png_set_invert_mono) PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_READ_BACKGROUND_SUPPORTED

#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_background) PNGARG((png_structp png_ptr,
   png_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma));
#endif
#define PNG_BACKGROUND_GAMMA_UNKNOWN 0
#define PNG_BACKGROUND_GAMMA_SCREEN  1
#define PNG_BACKGROUND_GAMMA_FILE    2
#define PNG_BACKGROUND_GAMMA_UNIQUE  3
#endif

#ifdef PNG_READ_16_TO_8_SUPPORTED

PNG_EXPORT(void,png_set_strip_16) PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED



PNG_EXPORT(void,png_set_quantize) PNGARG((png_structp png_ptr,
   png_colorp palette, int num_palette, int maximum_colors,
   png_uint_16p histogram, int full_quantize));
#endif

#define png_set_dither png_set_quantize

#ifdef PNG_READ_GAMMA_SUPPORTED

#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_gamma) PNGARG((png_structp png_ptr,
   double screen_gamma, double default_file_gamma));
#endif
#endif


#ifdef PNG_WRITE_FLUSH_SUPPORTED

PNG_EXPORT(void,png_set_flush) PNGARG((png_structp png_ptr, int nrows));

PNG_EXPORT(void,png_write_flush) PNGARG((png_structp png_ptr));
#endif


PNG_EXPORT(void,png_start_read_image) PNGARG((png_structp png_ptr));


PNG_EXPORT(void,png_read_update_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED

PNG_EXPORT(void,png_read_rows) PNGARG((png_structp png_ptr,
   png_bytepp row, png_bytepp display_row, png_uint_32 num_rows));
#endif

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED

PNG_EXPORT(void,png_read_row) PNGARG((png_structp png_ptr,
   png_bytep row,
   png_bytep display_row));
#endif

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED

PNG_EXPORT(void,png_read_image) PNGARG((png_structp png_ptr,
   png_bytepp image));
#endif


PNG_EXPORT(void,png_write_row) PNGARG((png_structp png_ptr,
   png_bytep row));


PNG_EXPORT(void,png_write_rows) PNGARG((png_structp png_ptr,
   png_bytepp row, png_uint_32 num_rows));


PNG_EXPORT(void,png_write_image) PNGARG((png_structp png_ptr,
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


PNG_EXPORT(void,png_write_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED

PNG_EXPORT(void,png_read_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif


PNG_EXPORT(void,png_destroy_info_struct) PNGARG((png_structp png_ptr,
   png_infopp info_ptr_ptr));


PNG_EXPORT(void,png_destroy_read_struct) PNGARG((png_structpp
   png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr));


PNG_EXPORT(void,png_destroy_write_struct)
   PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr));


PNG_EXPORT(void,png_set_crc_action) PNGARG((png_structp png_ptr,
   int crit_action, int ancil_action));










#define PNG_CRC_DEFAULT       0  /* error/quit          warn/discard data */
#define PNG_CRC_ERROR_QUIT    1  /* error/quit          error/quit        */
#define PNG_CRC_WARN_DISCARD  2  /* (INVALID)           warn/discard data */
#define PNG_CRC_WARN_USE      3  /* warn/use data       warn/use data     */
#define PNG_CRC_QUIET_USE     4  /* quiet/use data      quiet/use data    */
#define PNG_CRC_NO_CHANGE     5  /* use current value   use current value */












PNG_EXPORT(void,png_set_filter) PNGARG((png_structp png_ptr, int method,
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

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED 




























#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_filter_heuristics) PNGARG((png_structp png_ptr,
   int heuristic_method, int num_weights, png_doublep filter_weights,
   png_doublep filter_costs));
#endif
#endif 




#define PNG_FILTER_HEURISTIC_DEFAULT    0  /* Currently "UNWEIGHTED" */
#define PNG_FILTER_HEURISTIC_UNWEIGHTED 1  /* Used by libpng < 0.95 */
#define PNG_FILTER_HEURISTIC_WEIGHTED   2  /* Experimental feature */
#define PNG_FILTER_HEURISTIC_LAST       3  /* Not a valid value */








PNG_EXPORT(void,png_set_compression_level) PNGARG((png_structp png_ptr,
   int level));

PNG_EXPORT(void,png_set_compression_mem_level)
   PNGARG((png_structp png_ptr, int mem_level));

PNG_EXPORT(void,png_set_compression_strategy)
   PNGARG((png_structp png_ptr, int strategy));

PNG_EXPORT(void,png_set_compression_window_bits)
   PNGARG((png_structp png_ptr, int window_bits));

PNG_EXPORT(void,png_set_compression_method) PNGARG((png_structp png_ptr,
   int method));










#ifdef PNG_STDIO_SUPPORTED

PNG_EXPORT(void,png_init_io) PNGARG((png_structp png_ptr,
    png_FILE_p fp));
#endif









PNG_EXPORT(void,png_set_error_fn) PNGARG((png_structp png_ptr,
   png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warning_fn));


PNG_EXPORT(png_voidp,png_get_error_ptr) PNGARG((png_const_structp png_ptr));











PNG_EXPORT(void,png_set_write_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn));


PNG_EXPORT(void,png_set_read_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr read_data_fn));


PNG_EXPORT(png_voidp,png_get_io_ptr) PNGARG((png_structp png_ptr));

PNG_EXPORT(void,png_set_read_status_fn) PNGARG((png_structp png_ptr,
   png_read_status_ptr read_row_fn));

PNG_EXPORT(void,png_set_write_status_fn) PNGARG((png_structp png_ptr,
   png_write_status_ptr write_row_fn));

#ifdef PNG_USER_MEM_SUPPORTED

PNG_EXPORT(void,png_set_mem_fn) PNGARG((png_structp png_ptr,
   png_voidp mem_ptr, png_malloc_ptr malloc_fn, png_free_ptr free_fn));

PNG_EXPORT(png_voidp,png_get_mem_ptr) PNGARG((png_const_structp png_ptr));
#endif

#ifdef PNG_READ_USER_TRANSFORM_SUPPORTED
PNG_EXPORT(void,png_set_read_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr read_user_transform_fn));
#endif

#ifdef PNG_WRITE_USER_TRANSFORM_SUPPORTED
PNG_EXPORT(void,png_set_write_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr write_user_transform_fn));
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
PNG_EXPORT(void,png_set_user_transform_info) PNGARG((png_structp
   png_ptr, png_voidp user_transform_ptr, int user_transform_depth,
   int user_transform_channels));

PNG_EXPORT(png_voidp,png_get_user_transform_ptr)
   PNGARG((png_const_structp png_ptr));
#endif

#ifdef PNG_USER_CHUNKS_SUPPORTED
PNG_EXPORT(void,png_set_read_user_chunk_fn) PNGARG((png_structp png_ptr,
   png_voidp user_chunk_ptr, png_user_chunk_ptr read_user_chunk_fn));
PNG_EXPORT(png_voidp,png_get_user_chunk_ptr) PNGARG((png_const_structp
   png_ptr));
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED



PNG_EXPORT(void,png_set_progressive_read_fn) PNGARG((png_structp png_ptr,
   png_voidp progressive_ptr,
   png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
   png_progressive_end_ptr end_fn));

#ifdef PNG_READ_APNG_SUPPORTED
extern PNG_EXPORT(void,png_set_progressive_frame_fn) PNGARG((png_structp
   png_ptr,
   png_progressive_frame_ptr frame_info_fn,
   png_progressive_frame_ptr frame_end_fn));
#endif


PNG_EXPORT(png_voidp,png_get_progressive_ptr)
   PNGARG((png_const_structp png_ptr));


PNG_EXPORT(void,png_process_data) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep buffer, png_size_t buffer_size));




PNG_EXPORT(void,png_progressive_combine_row) PNGARG((png_structp png_ptr,
   png_bytep old_row, png_bytep new_row));
#endif 

PNG_EXPORT(png_voidp,png_malloc) PNGARG((png_structp png_ptr,
   png_alloc_size_t size)) PNG_ALLOCATED;

PNG_EXPORT(png_voidp,png_calloc) PNGARG((png_structp png_ptr,
   png_alloc_size_t size)) PNG_ALLOCATED;


PNG_EXPORT(png_voidp,png_malloc_warn) PNGARG((png_structp png_ptr,
   png_alloc_size_t size)) PNG_ALLOCATED;


PNG_EXPORT(void,png_free) PNGARG((png_structp png_ptr, png_voidp ptr));


PNG_EXPORT(void,png_free_data) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 free_me, int num));


PNG_EXPORT(void,png_data_freer) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int freer, png_uint_32 mask));

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
PNG_EXPORT(png_voidp,png_malloc_default) PNGARG((png_structp png_ptr,
   png_alloc_size_t size)) PNG_ALLOCATED;
PNG_EXPORT(void,png_free_default) PNGARG((png_structp png_ptr,
   png_voidp ptr));
#endif

#ifndef PNG_NO_ERROR_TEXT

PNG_EXPORT(void,png_error) PNGARG((png_structp png_ptr,
   png_const_charp error_message)) PNG_NORETURN;


PNG_EXPORT(void,png_chunk_error) PNGARG((png_structp png_ptr,
   png_const_charp error_message)) PNG_NORETURN;

#else

PNG_EXPORT(void,png_err) PNGARG((png_structp png_ptr)) PNG_NORETURN;
#endif


PNG_EXPORT(void,png_warning) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));


PNG_EXPORT(void,png_chunk_warning) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));

#ifdef PNG_BENIGN_ERRORS_SUPPORTED


PNG_EXPORT(void,png_benign_error) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));


PNG_EXPORT(void,png_chunk_benign_error) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));

PNG_EXPORT(void,png_set_benign_errors) PNGARG((png_structp
   png_ptr, int allowed));
#endif














PNG_EXPORT(png_uint_32,png_get_valid) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_uint_32 flag));


PNG_EXPORT(png_size_t,png_get_rowbytes) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr));

#ifdef PNG_INFO_IMAGE_SUPPORTED



PNG_EXPORT(png_bytepp,png_get_rows) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr));



PNG_EXPORT(void,png_set_rows) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers));
#endif


PNG_EXPORT(png_byte,png_get_channels) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr));

#ifdef PNG_EASY_ACCESS_SUPPORTED

PNG_EXPORT(png_uint_32, png_get_image_width) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_uint_32, png_get_image_height) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_byte, png_get_bit_depth) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_byte, png_get_color_type) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_byte, png_get_filter_type) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_byte, png_get_interlace_type) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_byte, png_get_compression_type) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


PNG_EXPORT(png_uint_32, png_get_pixels_per_meter) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));
PNG_EXPORT(png_uint_32, png_get_x_pixels_per_meter) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));
PNG_EXPORT(png_uint_32, png_get_y_pixels_per_meter) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));


#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(float, png_get_pixel_aspect_ratio) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));
#endif


PNG_EXPORT(png_int_32, png_get_x_offset_pixels) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));
PNG_EXPORT(png_int_32, png_get_y_offset_pixels) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));
PNG_EXPORT(png_int_32, png_get_x_offset_microns) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));
PNG_EXPORT(png_int_32, png_get_y_offset_microns) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr));

#endif 


PNG_EXPORT(png_bytep,png_get_signature) PNGARG((png_const_structp png_ptr,
   png_infop info_ptr));

#ifdef PNG_bKGD_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_bKGD) PNGARG((png_const_structp png_ptr,
   png_infop info_ptr, png_color_16p *background));
#endif

#ifdef PNG_bKGD_SUPPORTED
PNG_EXPORT(void,png_set_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p background));
#endif

#ifdef PNG_cHRM_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_cHRM) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, double *white_x, double *white_y, double *red_x,
   double *red_y, double *green_x, double *green_y, double *blue_x,
   double *blue_y));
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_cHRM_fixed) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_fixed_point *int_white_x, png_fixed_point
   *int_white_y, png_fixed_point *int_red_x, png_fixed_point *int_red_y,
   png_fixed_point *int_green_x, png_fixed_point *int_green_y, png_fixed_point
   *int_blue_x, png_fixed_point *int_blue_y));
#endif
#endif

#ifdef PNG_cHRM_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double white_x, double white_y, double red_x,
   double red_y, double green_x, double green_y, double blue_x, double blue_y));
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXPORT(void,png_set_cHRM_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point int_white_x, png_fixed_point int_white_y,
   png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
   int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
   png_fixed_point int_blue_y));
#endif
#endif

#ifdef PNG_gAMA_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_gAMA) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, double *file_gamma));
#endif
PNG_EXPORT(png_uint_32,png_get_gAMA_fixed) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_fixed_point *int_file_gamma));
#endif

#ifdef PNG_gAMA_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double file_gamma));
#endif
PNG_EXPORT(void,png_set_gAMA_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point int_file_gamma));
#endif

#ifdef PNG_hIST_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_hIST) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_uint_16p *hist));
#endif

#ifdef PNG_hIST_SUPPORTED
PNG_EXPORT(void,png_set_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p hist));
#endif

PNG_EXPORT(png_uint_32,png_get_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *width, png_uint_32 *height,
   int *bit_depth, int *color_type, int *interlace_method,
   int *compression_method, int *filter_method));

PNG_EXPORT(void,png_set_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_method, int compression_method,
   int filter_method));

#ifdef PNG_oFFs_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_oFFs) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_int_32 *offset_x, png_int_32 *offset_y,
   int *unit_type));
#endif

#ifdef PNG_oFFs_SUPPORTED
PNG_EXPORT(void,png_set_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_int_32 offset_x, png_int_32 offset_y,
   int unit_type));
#endif

#ifdef PNG_pCAL_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_pCAL) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_charp *purpose, png_int_32 *X0, png_int_32 *X1,
   int *type, int *nparams, png_charp *units, png_charpp *params));
#endif

#ifdef PNG_pCAL_SUPPORTED
PNG_EXPORT(void,png_set_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp purpose, png_int_32 X0, png_int_32 X1,
   int type, int nparams, png_charp units, png_charpp params));
#endif

#ifdef PNG_pHYs_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_pHYs) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type));
#endif

#ifdef PNG_pHYs_SUPPORTED
PNG_EXPORT(void,png_set_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 res_x, png_uint_32 res_y, int unit_type));
#endif

PNG_EXPORT(png_uint_32,png_get_PLTE) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_colorp *palette, int *num_palette));

PNG_EXPORT(void,png_set_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp palette, int num_palette));

#ifdef PNG_sBIT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_sBIT) PNGARG((png_const_structp png_ptr,
   png_infop info_ptr, png_color_8p *sig_bit));
#endif

#ifdef PNG_sBIT_SUPPORTED
PNG_EXPORT(void,png_set_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p sig_bit));
#endif

#ifdef PNG_sRGB_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_sRGB) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, int *intent));
#endif

#ifdef PNG_sRGB_SUPPORTED
PNG_EXPORT(void,png_set_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
PNG_EXPORT(void,png_set_sRGB_gAMA_and_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
#endif

#ifdef PNG_iCCP_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_iCCP) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_charpp name, int *compression_type,
   png_charpp profile, png_uint_32 *proflen));
   
#endif

#ifdef PNG_iCCP_SUPPORTED
PNG_EXPORT(void,png_set_iCCP) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp name, int compression_type,
   png_charp profile, png_uint_32 proflen));
   
#endif

#ifdef PNG_sPLT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_sPLT) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_sPLT_tpp entries));
#endif

#ifdef PNG_sPLT_SUPPORTED
PNG_EXPORT(void,png_set_sPLT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_sPLT_tp entries, int nentries));
#endif

#ifdef PNG_TEXT_SUPPORTED

PNG_EXPORT(png_uint_32,png_get_text) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_textp *text_ptr, int *num_text));
#endif








#ifdef PNG_TEXT_SUPPORTED
PNG_EXPORT(void,png_set_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp text_ptr, int num_text));
#endif

#ifdef PNG_tIME_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_tIME) PNGARG((png_const_structp png_ptr,
   png_infop info_ptr, png_timep *mod_time));
#endif

#ifdef PNG_tIME_SUPPORTED
PNG_EXPORT(void,png_set_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep mod_time));
#endif

#ifdef PNG_tRNS_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_tRNS) PNGARG((png_const_structp png_ptr,
   png_infop info_ptr, png_bytep *trans_alpha, int *num_trans,
   png_color_16p *trans_color));
#endif

#ifdef PNG_tRNS_SUPPORTED
PNG_EXPORT(void,png_set_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep trans_alpha, int num_trans,
   png_color_16p trans_color));
#endif

#ifdef PNG_tRNS_SUPPORTED
#endif

#ifdef PNG_sCAL_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_sCAL) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, int *unit, double *width, double *height));
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_sCAL_s) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, int *unit, png_charpp swidth, png_charpp sheight));
#endif
#endif
#endif 

#ifdef PNG_sCAL_SUPPORTED
#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXPORT(void,png_set_sCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int unit, double width, double height));
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXPORT(void,png_set_sCAL_s) PNGARG((png_structp png_ptr,
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









PNG_EXPORT(void, png_set_keep_unknown_chunks) PNGARG((png_structp
   png_ptr, int keep, png_bytep chunk_list, int num_chunks));
PNG_EXPORT(int,png_handle_as_unknown) PNGARG((png_structp png_ptr, png_bytep
   chunk_name));
#endif
#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
PNG_EXPORT(void, png_set_unknown_chunks) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_unknown_chunkp unknowns, int num_unknowns));
PNG_EXPORT(void, png_set_unknown_chunk_location)
   PNGARG((png_structp png_ptr, png_infop info_ptr, int chunk, int location));
PNG_EXPORT(png_uint_32,png_get_unknown_chunks) PNGARG((png_const_structp
   png_ptr, png_const_infop info_ptr, png_unknown_chunkpp entries));
#endif





PNG_EXPORT(void, png_set_invalid) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int mask));

#ifdef PNG_INFO_IMAGE_SUPPORTED

PNG_EXPORT(void, png_read_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
PNG_EXPORT(void, png_write_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
#endif

PNG_EXPORT(png_charp,png_get_copyright) PNGARG((png_const_structp png_ptr));
PNG_EXPORT(png_charp,png_get_header_ver) PNGARG((png_const_structp png_ptr));
PNG_EXPORT(png_charp,png_get_header_version) PNGARG((png_const_structp
    png_ptr));
PNG_EXPORT(png_charp,png_get_libpng_ver) PNGARG((png_const_structp png_ptr));

#ifdef PNG_MNG_FEATURES_SUPPORTED
PNG_EXPORT(png_uint_32,png_permit_mng_features) PNGARG((png_structp
   png_ptr, png_uint_32 mng_features_permitted));
#endif


#define PNG_HANDLE_CHUNK_AS_DEFAULT   0
#define PNG_HANDLE_CHUNK_NEVER        1
#define PNG_HANDLE_CHUNK_IF_SAFE      2
#define PNG_HANDLE_CHUNK_ALWAYS       3




#ifdef PNG_ERROR_NUMBERS_SUPPORTED
PNG_EXPORT(void,png_set_strip_error_numbers) PNGARG((png_structp
   png_ptr, png_uint_32 strip_mode));
#endif


#ifdef PNG_SET_USER_LIMITS_SUPPORTED
PNG_EXPORT(void,png_set_user_limits) PNGARG((png_structp
   png_ptr, png_uint_32 user_width_max, png_uint_32 user_height_max));
PNG_EXPORT(png_uint_32,png_get_user_width_max) PNGARG((png_const_structp
   png_ptr));
PNG_EXPORT(png_uint_32,png_get_user_height_max) PNGARG((png_const_structp
   png_ptr));

PNG_EXPORT(void,png_set_chunk_cache_max) PNGARG((png_structp
   png_ptr, png_uint_32 user_chunk_cache_max));
PNG_EXPORT(png_uint_32,png_get_chunk_cache_max)
   PNGARG((png_const_structp png_ptr));

PNG_EXPORT(void,png_set_chunk_malloc_max) PNGARG((png_structp
   png_ptr, png_alloc_size_t user_chunk_cache_max));
PNG_EXPORT(png_alloc_size_t,png_get_chunk_malloc_max)
   PNGARG((png_const_structp png_ptr));
#endif

#if defined(PNG_INCH_CONVERSIONS) && defined(PNG_FLOATING_POINT_SUPPORTED)
PNG_EXPORT(png_uint_32,png_get_pixels_per_inch)
   PNGARG((png_const_structp png_ptr, png_const_infop info_ptr));

PNG_EXPORT(png_uint_32,png_get_x_pixels_per_inch)
   PNGARG((png_const_structp png_ptr, png_const_infop info_ptr));

PNG_EXPORT(png_uint_32,png_get_y_pixels_per_inch)
   PNGARG((png_const_structp png_ptr, png_const_infop info_ptr));

PNG_EXPORT(float,png_get_x_offset_inches) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr));

PNG_EXPORT(float,png_get_y_offset_inches) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr));

#ifdef PNG_pHYs_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_pHYs_dpi) PNGARG((png_const_structp png_ptr,
   png_const_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y,
   int *unit_type));
#endif 
#endif  


#ifdef PNG_IO_STATE_SUPPORTED
PNG_EXPORT(png_uint_32,png_get_io_state) PNGARG((png_const_structp png_ptr));

PNG_EXPORT(png_bytep,png_get_io_chunk_name)
   PNGARG((png_structp png_ptr));


#define PNG_IO_NONE        0x0000   /* no I/O at this moment */
#define PNG_IO_READING     0x0001   /* currently reading */
#define PNG_IO_WRITING     0x0002   /* currently writing */
#define PNG_IO_SIGNATURE   0x0010   /* currently at the file signature */
#define PNG_IO_CHUNK_HDR   0x0020   /* currently at the chunk header */
#define PNG_IO_CHUNK_DATA  0x0040   /* currently at the chunk data */
#define PNG_IO_CHUNK_CRC   0x0080   /* currently at the chunk crc */
#define PNG_IO_MASK_OP     0x000f   /* current operation: reading/writing */
#define PNG_IO_MASK_LOC    0x00f0   /* current location: sig/hdr/data/crc */
#endif 





#ifdef PNG_READ_COMPOSITE_NODIV_SUPPORTED












 

#  define png_composite(composite, fg, alpha, bg)         \
     { png_uint_16 temp = (png_uint_16)((png_uint_16)(fg) \
           * (png_uint_16)(alpha)                         \
           + (png_uint_16)(bg)*(png_uint_16)(255          \
           - (png_uint_16)(alpha)) + (png_uint_16)128);   \
       (composite) = (png_byte)((temp + (temp >> 8)) >> 8); }

#  define png_composite_16(composite, fg, alpha, bg)       \
     { png_uint_32 temp = (png_uint_32)((png_uint_32)(fg)  \
           * (png_uint_32)(alpha)                          \
           + (png_uint_32)(bg)*(png_uint_32)(65535L        \
           - (png_uint_32)(alpha)) + (png_uint_32)32768L); \
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

#ifdef PNG_USE_READ_MACROS




#  define png_get_uint_32(buf) \
     (((png_uint_32)(*(buf)) << 24) + \
      ((png_uint_32)(*((buf) + 1)) << 16) + \
      ((png_uint_32)(*((buf) + 2)) << 8) + \
      ((png_uint_32)(*((buf) + 3))))

  








#  define png_get_uint_16(buf) \
     ((png_uint_16) \
       (((unsigned int)(*(buf)) << 8) + \
        ((unsigned int)(*((buf) + 1)))))

#  define png_get_int_32(buf) \
     ((png_int_32)((*(buf) & 0x80) \
      ? -((png_int_32)((png_get_uint_32(buf) ^ 0xffffffffL) + 1)) \
      : (png_int_32)png_get_uint_32(buf)))
#else
PNG_EXPORT(png_uint_32,png_get_uint_32) PNGARG((png_bytep buf));
PNG_EXPORT(png_uint_16,png_get_uint_16) PNGARG((png_bytep buf));
#ifdef PNG_GET_INT_32_SUPPORTED
PNG_EXPORT(png_int_32,png_get_int_32) PNGARG((png_bytep buf));
#endif
#endif
PNG_EXPORT(png_uint_32,png_get_uint_31)
  PNGARG((png_structp png_ptr, png_bytep buf));



PNG_EXPORT(void,png_save_uint_32)
   PNGARG((png_bytep buf, png_uint_32 i));
PNG_EXPORT(void,png_save_int_32)
   PNGARG((png_bytep buf, png_int_32 i));





PNG_EXPORT(void,png_save_uint_16)
   PNGARG((png_bytep buf, unsigned int i));







#define PNG_HAVE_IHDR               0x01
#define PNG_HAVE_PLTE               0x02
#define PNG_HAVE_IDAT               0x04
#define PNG_AFTER_IDAT              0x08 /* Have complete zlib datastream */
#define PNG_HAVE_IEND               0x10
#define PNG_HAVE_gAMA               0x20
#define PNG_HAVE_cHRM               0x40

#ifdef __cplusplus
}
#endif

#endif

#endif
