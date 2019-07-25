



















#ifndef PNGSTRUCT_H
#define PNGSTRUCT_H




#include "zlib.h"

struct png_struct_def
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf longjmp_buffer;    
   png_longjmp_ptr longjmp_fn;
#endif
   png_error_ptr error_fn;    
#ifdef PNG_WARNINGS_SUPPORTED
   png_error_ptr warning_fn;  
#endif
   png_voidp error_ptr;       
   png_rw_ptr write_data_fn;  
   png_rw_ptr read_data_fn;   
   png_voidp io_ptr;          

#ifdef PNG_READ_USER_TRANSFORM_SUPPORTED
   png_user_transform_ptr read_user_transform_fn; 
#endif

#ifdef PNG_WRITE_USER_TRANSFORM_SUPPORTED
   png_user_transform_ptr write_user_transform_fn; 
#endif


#ifdef PNG_USER_TRANSFORM_PTR_SUPPORTED
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
   uInt zbuf_size;            
#ifdef PNG_WRITE_SUPPORTED




#define PNG_ZLIB_UNINITIALIZED 0
#define PNG_ZLIB_FOR_IDAT      1
#define PNG_ZLIB_FOR_TEXT      2 /* anything other than IDAT */
#define PNG_ZLIB_USE_MASK      3 /* bottom two bits */
#define PNG_ZLIB_IN_USE        4 /* a flag value */

   png_uint_32 zlib_state;       


   int zlib_level;            
   int zlib_method;           
   int zlib_window_bits;      
   int zlib_mem_level;        
   int zlib_strategy;         
#endif

#if defined(PNG_WRITE_COMPRESSED_TEXT_SUPPORTED) || \
    defined(PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED)
   int zlib_text_level;            
   int zlib_text_method;           
   int zlib_text_window_bits;      
   int zlib_text_mem_level;        
   int zlib_text_strategy;         
#endif


   png_uint_32 width;         
   png_uint_32 height;        
   png_uint_32 num_rows;      
   png_uint_32 usr_width;     
   png_size_t rowbytes;       
   png_uint_32 iwidth;        
   png_uint_32 row_number;    
   png_uint_32 chunk_name;    
   png_bytep prev_row;        


   png_bytep row_buf;         


   png_bytep sub_row;         
   png_bytep up_row;          
   png_bytep avg_row;         
   png_bytep paeth_row;       
   png_size_t info_rowbytes;  

   png_uint_32 idat_size;     
   png_uint_32 crc;           
   png_colorp palette;        
   png_uint_16 num_palette;   
   png_uint_16 num_trans;     
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
   png_byte maximum_pixel_depth;
                              
   png_byte transformed_pixel_depth;
                              
   png_byte io_chunk_string[5];
                              

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
   png_uint_16 filler;           
#endif

#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
   defined(PNG_READ_ALPHA_MODE_SUPPORTED)
   png_byte background_gamma_type;
   png_fixed_point background_gamma;
   png_color_16 background;   
#ifdef PNG_READ_GAMMA_SUPPORTED
   png_color_16 background_1; 
#endif
#endif 

#ifdef PNG_WRITE_FLUSH_SUPPORTED
   png_flush_ptr output_flush_fn; 
   png_uint_32 flush_dist;    
   png_uint_32 flush_rows;    
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
   int gamma_shift;      
   png_fixed_point gamma;        
   png_fixed_point screen_gamma; 

   png_bytep gamma_table;     
   png_uint_16pp gamma_16_table; 
#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   png_bytep gamma_from_1;    
   png_bytep gamma_to_1;      
   png_uint_16pp gamma_16_from_1; 
   png_uint_16pp gamma_16_to_1; 
#endif 
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_sBIT_SUPPORTED)
   png_color_8 sig_bit;       
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
   png_color_8 shift;         
#endif

#if defined(PNG_tRNS_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) \
 || defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep trans_alpha;           
   png_color_16 trans_color;  
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

#  ifdef PNG_TEXT_SUPPORTED
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

#ifdef PNG_READ_QUANTIZE_SUPPORTED
   png_bytep palette_lookup; 
   png_bytep quantize_index; 
#endif

#if defined(PNG_READ_QUANTIZE_SUPPORTED) || defined(PNG_hIST_SUPPORTED)
   png_uint_16p hist;                
#endif

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   png_byte heuristic_method;        
   png_byte num_prev_filters;        
   png_bytep prev_filters;           
   png_uint_16p filter_weights;      
   png_uint_16p inv_filter_weights;  
   png_uint_16p filter_costs;        
   png_uint_16p inv_filter_costs;    
#endif

#ifdef PNG_TIME_RFC1123_SUPPORTED
   char time_buffer[29]; 
#endif



   png_uint_32 free_me;    

#ifdef PNG_USER_CHUNKS_SUPPORTED
   png_voidp user_chunk_ptr;
   png_user_chunk_ptr read_user_chunk_fn; 
#endif

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   int num_chunk_list;
   png_bytep chunk_list;
#endif

#ifdef PNG_READ_sRGB_SUPPORTED
   
   png_byte is_sRGB;
#endif


#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
   png_byte rgb_to_gray_status;
   
   png_byte rgb_to_gray_coefficients_set;
   
   png_uint_16 rgb_to_gray_red_coeff;
   png_uint_16 rgb_to_gray_green_coeff;
   
#endif


#if defined(PNG_MNG_FEATURES_SUPPORTED)

   png_uint_32 mng_features_permitted;
#endif


#ifdef PNG_MNG_FEATURES_SUPPORTED
   png_byte filter_type;
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




#ifdef PNG_USER_MEM_SUPPORTED
   png_voidp mem_ptr;             
   png_malloc_ptr malloc_fn;      
   png_free_ptr free_fn;          
#endif


   png_bytep big_row_buf;         

#ifdef PNG_READ_QUANTIZE_SUPPORTED

   png_bytep quantize_sort;          
   png_bytep index_to_palette;       

   png_bytep palette_to_index;       

#endif


   png_byte compression_type;

#ifdef PNG_USER_LIMITS_SUPPORTED
   png_uint_32 user_width_max;
   png_uint_32 user_height_max;

   


   png_uint_32 user_chunk_cache_max;

   


   png_alloc_size_t user_chunk_malloc_max;
#endif


#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
   
   png_unknown_chunk unknown_chunk;
#endif


  png_size_t old_big_row_buf_size;


  png_charp chunkdata;  

#ifdef PNG_IO_STATE_SUPPORTED

   png_uint_32 io_state;
#endif


   png_bytep big_prev_row;

   void (*read_filter[PNG_FILTER_VALUE_LAST-1])(png_row_infop row_info,
      png_bytep row, png_const_bytep prev_row);
};
#endif 
