
















#ifndef _O_THEORA_H_
#define _O_THEORA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>	

#include <ogg/ogg.h>






















































































































typedef struct {
    int   y_width;      
    int   y_height;     
    int   y_stride;     

    int   uv_width;     
    int   uv_height;    
    int   uv_stride;    
    unsigned char *y;   
    unsigned char *u;   
    unsigned char *v;   

} yuv_buffer;




typedef enum {
  OC_CS_UNSPECIFIED,    
  OC_CS_ITU_REC_470M,   
  OC_CS_ITU_REC_470BG,  
  OC_CS_NSPACES         
} theora_colorspace;








typedef enum {
  OC_PF_420,    
  OC_PF_RSVD,   
  OC_PF_422,    
  OC_PF_444,    
} theora_pixelformat;






















typedef struct {
  ogg_uint32_t  width;		
  ogg_uint32_t  height;		
  ogg_uint32_t  frame_width;	
  ogg_uint32_t  frame_height;	
  ogg_uint32_t  offset_x;	
  ogg_uint32_t  offset_y;	
  ogg_uint32_t  fps_numerator;	    
  ogg_uint32_t  fps_denominator;    
  ogg_uint32_t  aspect_numerator;   
  ogg_uint32_t  aspect_denominator; 
  theora_colorspace colorspace;	    
  int           target_bitrate;	    
  int           quality;  
  int           quick_p;  

  
  unsigned char version_major;
  unsigned char version_minor;
  unsigned char version_subminor;

  void *codec_setup;

  
  int           dropframes_p;
  int           keyframe_auto_p;
  ogg_uint32_t  keyframe_frequency;
  ogg_uint32_t  keyframe_frequency_force;  

  ogg_uint32_t  keyframe_data_target_bitrate;
  ogg_int32_t   keyframe_auto_threshold;
  ogg_uint32_t  keyframe_mindistance;
  ogg_int32_t   noise_sensitivity;
  ogg_int32_t   sharpness;

  theora_pixelformat pixelformat;	

} theora_info;



typedef struct{
  theora_info *i;
  ogg_int64_t granulepos;

  void *internal_encode;
  void *internal_decode;

} theora_state;




















typedef struct theora_comment{
  char **user_comments;         
  int   *comment_lengths;       
  int    comments;              
  char  *vendor;                

} theora_comment;
























#define TH_DECCTL_GET_PPLEVEL_MAX (1)






#define TH_DECCTL_SET_PPLEVEL (3)













#define TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE (4)








#define TH_DECCTL_SET_GRANPOS (5)
















#define TH_ENCCTL_SET_QUANT_PARAMS (2)




























#define TH_ENCCTL_SET_VP3_COMPATIBLE (10)














#define TH_ENCCTL_GET_SPLEVEL_MAX (12)














#define TH_ENCCTL_SET_SPLEVEL (14)



#define OC_FAULT       -1       /**< General failure */
#define OC_EINVAL      -10      /**< Library encountered invalid internal data */
#define OC_DISABLED    -11      /**< Requested action is disabled */
#define OC_BADHEADER   -20      /**< Header packet was corrupt/invalid */
#define OC_NOTFORMAT   -21      /**< Packet is not a theora packet */
#define OC_VERSION     -22      /**< Bitstream version is not handled */
#define OC_IMPL        -23      /**< Feature or action not implemented */
#define OC_BADPACKET   -24      /**< Packet is corrupt */
#define OC_NEWPACKET   -25      /**< Packet is an (ignorable) unhandled extension */
#define OC_DUPFRAME    1        /**< Packet is a dropped frame */





extern const char *theora_version_string(void);










extern ogg_uint32_t theora_version_number(void);







extern int theora_encode_init(theora_state *th, theora_info *ti);











extern int theora_encode_YUVin(theora_state *t, yuv_buffer *yuv);













extern int theora_encode_packetout( theora_state *t, int last_p,
                                    ogg_packet *op);











extern int theora_encode_header(theora_state *t, ogg_packet *op);












extern int theora_encode_comment(theora_comment *tc, ogg_packet *op);











extern int theora_encode_tables(theora_state *t, ogg_packet *op);










































extern int theora_decode_header(theora_info *ci, theora_comment *cc,
                                ogg_packet *op);









extern int theora_decode_init(theora_state *th, theora_info *c);








extern int theora_decode_packetin(theora_state *th,ogg_packet *op);











extern int theora_decode_YUVout(theora_state *th,yuv_buffer *yuv);













extern int theora_packet_isheader(ogg_packet *op);











extern int theora_packet_iskeyframe(ogg_packet *op);


























int theora_granule_shift(theora_info *ti);

















extern ogg_int64_t theora_granule_frame(theora_state *th,ogg_int64_t granulepos);
















extern double theora_granule_time(theora_state *th,ogg_int64_t granulepos);







extern void theora_info_init(theora_info *c);






extern void theora_info_clear(theora_info *c);





extern void theora_clear(theora_state *t);





extern void theora_comment_init(theora_comment *tc);













extern void theora_comment_add(theora_comment *tc, char *comment);













extern void theora_comment_add_tag(theora_comment *tc,
                                       char *tag, char *value);















extern char *theora_comment_query(theora_comment *tc, char *tag, int count);










extern int   theora_comment_query_count(theora_comment *tc, char *tag);





extern void  theora_comment_clear(theora_comment *tc);









extern int theora_control(theora_state *th,int req,void *buf,size_t buf_sz);

 

#ifdef __cplusplus
}
#endif 

#endif
