



































#ifndef _GIF_H_
#define _GIF_H_

#define MAX_LZW_BITS          12
#define MAX_BITS            4097 /* 2^MAX_LZW_BITS+1 */
#define MAX_COLORS           256
#define MAX_HOLD_SIZE        256

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
    PRUint32 bytes_to_consume;      
    PRUint32 bytes_in_hold;         

    
    PRUint8 *stackp;              
    int datasize;
    int codesize;
    int codemask;
    int avail;                  
    int oldcode;
    PRUint8 firstchar;
    int count;                  
    int bits;                   
    int32 datum;                

    
    int ipass;                  
    PRUintn rows_remaining;        
    PRUintn irow;                  
    PRUint8 *rowp;                 

    
    PRUintn x_offset, y_offset;    
    PRUintn height, width;
    int tpixel;                 
    PRInt32 disposal_method;    
    PRUint32 *local_colormap;   
    int local_colormap_size;    
    PRUint32 delay_time;        


    
    int version;                
    PRUintn screen_width;       
    PRUintn screen_height;
    PRUint32 global_colormap_depth;  
    int images_decoded;         
    int loop_count;             


    bool progressive_display;    
    bool interlaced;             
    bool is_transparent;         

    PRUint16  prefix[MAX_BITS];          
    PRUint8   hold[MAX_HOLD_SIZE];       
    PRUint32  global_colormap[MAX_COLORS];   
    PRUint8   suffix[MAX_BITS];          
    PRUint8   stack[MAX_BITS];           

} gif_struct;

#endif

