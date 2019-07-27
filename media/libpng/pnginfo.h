












 





































#ifndef PNGINFO_H
#define PNGINFO_H

struct png_info_def
{
   
   png_uint_32 width;  
   png_uint_32 height; 
   png_uint_32 valid;  
   png_size_t rowbytes; 
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

#ifdef PNG_READ_SUPPORTED
   
   png_byte signature[8];   
#endif

   





#if defined(PNG_COLORSPACE_SUPPORTED) || defined(PNG_GAMMA_SUPPORTED)
   









   png_colorspace colorspace;
#endif

#ifdef PNG_iCCP_SUPPORTED
   
   png_charp iccp_name;     
   png_bytep iccp_profile;  
   png_uint_32 iccp_proflen;  
#endif

#ifdef PNG_TEXT_SUPPORTED
   







   int num_text; 
   int max_text; 
   png_textp text; 
#endif 

#ifdef PNG_tIME_SUPPORTED
   


   png_time mod_time;
#endif

#ifdef PNG_sBIT_SUPPORTED
   





   png_color_8 sig_bit; 
#endif

#if defined(PNG_tRNS_SUPPORTED) || defined(PNG_READ_EXPAND_SUPPORTED) || \
defined(PNG_READ_BACKGROUND_SUPPORTED)
   








   png_bytep trans_alpha;    
   png_color_16 trans_color; 
#endif

#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   





   png_color_16 background;
#endif

#ifdef PNG_oFFs_SUPPORTED
   




   png_int_32 x_offset; 
   png_int_32 y_offset; 
   png_byte offset_unit_type; 
#endif

#ifdef PNG_pHYs_SUPPORTED
   



   png_uint_32 x_pixels_per_unit; 
   png_uint_32 y_pixels_per_unit; 
   png_byte phys_unit_type; 
#endif

#ifdef PNG_hIST_SUPPORTED
   





   png_uint_16p hist;
#endif

#ifdef PNG_pCAL_SUPPORTED
   










   png_charp pcal_purpose;  
   png_int_32 pcal_X0;      
   png_int_32 pcal_X1;      
   png_charp pcal_units;    
   png_charpp pcal_params;  
   png_byte pcal_type;      
   png_byte pcal_nparams;   
#endif


   png_uint_32 free_me;     

#ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
   
   png_unknown_chunkp unknown_chunks;

   


   int                unknown_chunks_num;
#endif

#ifdef PNG_sPLT_SUPPORTED
   
   png_sPLT_tp splt_palettes;
   int         splt_palettes_num; 
#endif

#ifdef PNG_sCAL_SUPPORTED
   






   png_byte scal_unit;         
   png_charp scal_s_width;     
   png_charp scal_s_height;    
#endif

#ifdef PNG_INFO_IMAGE_SUPPORTED
   

   
   png_bytepp row_pointers;        
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

};
#endif 
