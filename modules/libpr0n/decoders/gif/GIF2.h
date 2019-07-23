



































#ifndef _GIF_H_
#define _GIF_H_

#define MAX_LZW_BITS          12
#define MAX_BITS            4097 /* 2^MAX_LZW_BITS+1 */
#define MAX_COLORS           256
#define MAX_HOLD_SIZE        256





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
    gif_oom,
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
    PRUint8 *rowbuf;              
    PRUint8 *rowend;              
    PRUint8 *rowp;                

    
    PRUintn x_offset, y_offset;    
    PRUintn height, width;
    int tpixel;                 
    PRInt32 disposal_method;    
    PRUint8 *local_colormap;    
    int local_colormap_size;    
    PRUint32 delay_time;        


    
    int version;                
    PRUintn screen_width;       
    PRUintn screen_height;
    int global_colormap_size;   
    int images_decoded;         
    int loop_count;             


    PRPackedBool progressive_display;    
    PRPackedBool interlaced;             
    PRPackedBool is_transparent;         
    PRPackedBool is_local_colormap_defined;

    PRUint16  prefix[MAX_BITS];          
    PRUint8   hold[MAX_HOLD_SIZE];       
    PRUint8   global_colormap[3*MAX_COLORS];   
    PRUint8   suffix[MAX_BITS];          
    PRUint8   stack[MAX_BITS];           

} gif_struct;

#endif

