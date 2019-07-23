




























































#if !defined(_O_THEORA_CODEC_H_)
# define _O_THEORA_CODEC_H_ (1)
# include <ogg/ogg.h>

#if defined(__cplusplus)
extern "C" {
#endif






#define TH_EFAULT     (-1)

#define TH_EINVAL     (-10)

#define TH_EBADHEADER (-20)

#define TH_ENOTFORMAT (-21)

#define TH_EVERSION   (-22)

#define TH_EIMPL      (-23)

#define TH_EBADPACKET (-24)



#define TH_DUPFRAME   (1)






typedef enum{
  

  TH_CS_UNSPECIFIED,
  
  TH_CS_ITU_REC_470M,
  
  TH_CS_ITU_REC_470BG,
  
  TH_CS_NSPACES
}th_colorspace;





typedef enum{
  


  TH_PF_420,
  
  TH_PF_RSVD,
  


  TH_PF_422,
  

  TH_PF_444,
  
  TH_PF_NFORMATS
}th_pixel_fmt;











typedef struct{
  
  int            width;
  
  int            height;
  
  int            stride;
  
  unsigned char *data;
}th_img_plane;
















typedef th_img_plane th_ycbcr_buffer[3];



































typedef struct{
  

  
  unsigned char version_major;
  unsigned char version_minor;
  unsigned char version_subminor;
  
  

  ogg_uint32_t  frame_width;
  

  ogg_uint32_t  frame_height;
  

  ogg_uint32_t  pic_width;
  

  ogg_uint32_t  pic_height;
  


  ogg_uint32_t  pic_x;
  






  ogg_uint32_t  pic_y;
  


  
  ogg_uint32_t  fps_numerator;
  ogg_uint32_t  fps_denominator;
  
  







  
  ogg_uint32_t  aspect_numerator;
  ogg_uint32_t  aspect_denominator;
  
  
  th_colorspace colorspace;
  
  th_pixel_fmt  pixel_fmt;
  


  int           target_bitrate;
  




  













  int           quality;
  












  int           keyframe_granule_shift;
}th_info;

























typedef struct th_comment{
  
  char **user_comments;
  
  int   *comment_lengths;
  
  int    comments;
  

  char  *vendor;
}th_comment;




typedef unsigned char th_quant_base[64];


typedef struct{
  
  int                  nranges;
  

  const int           *sizes;
  

  const th_quant_base *base_matrices;
}th_quant_ranges;


























































typedef struct{
  
  ogg_uint16_t    dc_scale[64];
  
  ogg_uint16_t    ac_scale[64];
  
  unsigned char   loop_filter_limits[64];
  
  th_quant_ranges qi_ranges[2][3];
}th_quant_info;




#define TH_NHUFFMAN_TABLES (80)

#define TH_NDCT_TOKENS     (32)












typedef struct{
  

  ogg_uint32_t pattern;
  

  int          nbits;
}th_huff_code;










extern const char *th_version_string(void);









extern ogg_uint32_t th_version_number(void);









extern ogg_int64_t th_granule_frame(void *_encdec,ogg_int64_t _granpos);











extern double th_granule_time(void *_encdec,ogg_int64_t _granpos);









extern int th_packet_isheader(ogg_packet *_op);










extern int th_packet_iskeyframe(ogg_packet *_op);









extern void th_info_init(th_info *_info);




extern void th_info_clear(th_info *_info);





extern void th_comment_init(th_comment *_tc);









extern void th_comment_add(th_comment *_tc, char *_comment);










extern void th_comment_add_tag(th_comment *_tc,char *_tag,char *_val);















extern char *th_comment_query(th_comment *_tc,char *_tag,int _count);







extern int th_comment_query_count(th_comment *_tc,char *_tag);





extern void th_comment_clear(th_comment *_tc);





#if defined(__cplusplus)
}
#endif

#endif
