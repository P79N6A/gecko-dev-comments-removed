




#ifndef mozilla_image_decoders_GIF2_H
#define mozilla_image_decoders_GIF2_H

#define MAX_LZW_BITS          12
#define MAX_BITS            4097 // 2^MAX_LZW_BITS+1
#define MAX_COLORS           256
#define MIN_HOLD_SIZE        256

enum { GIF_TRAILER                     = 0x3B }; 
enum { GIF_IMAGE_SEPARATOR             = 0x2C }; 
enum { GIF_EXTENSION_INTRODUCER        = 0x21 }; 
enum { GIF_GRAPHIC_CONTROL_LABEL       = 0xF9 };
enum { GIF_COMMENT_LABEL               = 0xFE };
enum { GIF_PLAIN_TEXT_LABEL            = 0x01 };
enum { GIF_APPLICATION_EXTENSION_LABEL = 0xFF };





typedef enum {
    gif_type,
    gif_global_header,
    gif_global_colormap,
    gif_image_start,
    gif_image_header,
    gif_image_header_continue,
    gif_image_colormap,
    gif_image_body,
    gif_lzw_start,
    gif_lzw,
    gif_sub_block,
    gif_extension,
    gif_control_extension,
    gif_consume_block,
    gif_skip_block,
    gif_done,
    gif_error,
    gif_comment_extension,
    gif_application_extension,
    gif_netscape_extension_block,
    gif_consume_netscape_extension,
    gif_consume_comment
} gstate;


typedef struct gif_struct {
    
    gstate state;               
    uint32_t bytes_to_consume;  
    uint32_t bytes_in_hold;     

    
    uint8_t* stackp;            
    int datasize;
    int codesize;
    int codemask;
    int avail;                  
    int oldcode;
    uint8_t firstchar;
    int count;                  
    int bits;                   
    int32_t datum;              

    
    int ipass;                  
    unsigned rows_remaining;    
    unsigned irow;              
    uint8_t* rowp;              

    
    unsigned x_offset, y_offset; 
    unsigned height, width;
    int tpixel;                 
    int32_t disposal_method;    
    uint32_t* local_colormap;   
    int local_colormap_size;    
    uint32_t delay_time;        
                                

    
    int version;                
    unsigned screen_width;      
    unsigned screen_height;
    uint32_t global_colormap_depth; 
    int images_decoded;         
    int loop_count;             
                                
                                

    bool progressive_display;   
    bool interlaced;            
    bool is_transparent;        

    uint16_t  prefix[MAX_BITS];            
    uint8_t*  hold;                        
    uint32_t  global_colormap[MAX_COLORS]; 
                                           
    uint8_t   suffix[MAX_BITS];            
    uint8_t   stack[MAX_BITS];             

} gif_struct;

#endif 

