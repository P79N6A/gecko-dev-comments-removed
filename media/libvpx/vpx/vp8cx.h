








#ifndef VPX_VP8CX_H_
#define VPX_VP8CX_H_






#include "./vp8.h"
#include "./vpx_encoder.h"






#ifdef __cplusplus
extern "C" {
#endif






extern vpx_codec_iface_t  vpx_codec_vp8_cx_algo;
extern vpx_codec_iface_t *vpx_codec_vp8_cx(void);







extern vpx_codec_iface_t  vpx_codec_vp9_cx_algo;
extern vpx_codec_iface_t *vpx_codec_vp9_cx(void);













#define VP8_EFLAG_NO_REF_LAST      (1<<16)








#define VP8_EFLAG_NO_REF_GF        (1<<17)








#define VP8_EFLAG_NO_REF_ARF       (1<<21)







#define VP8_EFLAG_NO_UPD_LAST      (1<<18)







#define VP8_EFLAG_NO_UPD_GF        (1<<22)







#define VP8_EFLAG_NO_UPD_ARF       (1<<23)







#define VP8_EFLAG_FORCE_GF         (1<<19)







#define VP8_EFLAG_FORCE_ARF        (1<<24)







#define VP8_EFLAG_NO_UPD_ENTROPY   (1<<20)









enum vp8e_enc_control_id {
  



  VP8E_UPD_ENTROPY           = 5,

  



  VP8E_UPD_REFERENCE,

  



  VP8E_USE_REFERENCE,

  



  VP8E_SET_ROI_MAP,

  



  VP8E_SET_ACTIVEMAP,

  



  VP8E_SET_SCALEMODE         = 11,

  










  VP8E_SET_CPUUSED           = 13,

  



  VP8E_SET_ENABLEAUTOALTREF,

  






  VP8E_SET_NOISE_SENSITIVITY,

  



  VP8E_SET_SHARPNESS,

  



  VP8E_SET_STATIC_THRESHOLD,

  



  VP8E_SET_TOKEN_PARTITIONS,

  





  VP8E_GET_LAST_QUANTIZER,

  






  VP8E_GET_LAST_QUANTIZER_64,

  



  VP8E_SET_ARNR_MAXFRAMES,

  



  VP8E_SET_ARNR_STRENGTH,

  
  VP8E_SET_ARNR_TYPE,

  



  VP8E_SET_TUNING,

  







  VP8E_SET_CQ_LEVEL,

  












  VP8E_SET_MAX_INTRA_BITRATE_PCT,

  



  VP8E_SET_FRAME_FLAGS,

  












  VP9E_SET_MAX_INTER_BITRATE_PCT,

  












  VP9E_SET_GF_CBR_BOOST_PCT,

  








  VP8E_SET_TEMPORAL_LAYER_ID,

  





  VP8E_SET_SCREEN_CONTENT_MODE,

  












  VP9E_SET_LOSSLESS,

  




















  VP9E_SET_TILE_COLUMNS,

  

















  VP9E_SET_TILE_ROWS,

  











  VP9E_SET_FRAME_PARALLEL_DECODING,

  










  VP9E_SET_AQ_MODE,

  












  VP9E_SET_FRAME_PERIODIC_BOOST,

  





  VP9E_SET_NOISE_SENSITIVITY,

  






  VP9E_SET_SVC,

  





  VP9E_SET_SVC_PARAMETERS,

  






  VP9E_SET_SVC_LAYER_ID,

  






  VP9E_SET_TUNE_CONTENT,

  





  VP9E_GET_SVC_LAYER_ID,

  





  VP9E_REGISTER_CX_CALLBACK,

  












  VP9E_SET_COLOR_SPACE,

  








  VP9E_SET_TEMPORAL_LAYERING_MODE,

  



  VP9E_GET_ACTIVEMAP,
};





typedef enum vpx_scaling_mode_1d {
  VP8E_NORMAL      = 0,
  VP8E_FOURFIVE    = 1,
  VP8E_THREEFIVE   = 2,
  VP8E_ONETWO      = 3
} VPX_SCALING_MODE;







typedef enum vp9e_temporal_layering_mode {
  


  VP9E_TEMPORAL_LAYERING_MODE_NOLAYERING   = 0,

  



  VP9E_TEMPORAL_LAYERING_MODE_BYPASS       = 1,

  

  VP9E_TEMPORAL_LAYERING_MODE_0101         = 2,

  

  VP9E_TEMPORAL_LAYERING_MODE_0212         = 3
} VP9E_TEMPORAL_LAYERING_MODE;







typedef struct vpx_roi_map {
  
  unsigned char *roi_map;
  unsigned int rows;       
  unsigned int cols;       
  
  
  
  int delta_q[4];          
  int delta_lf[4];         
  
  unsigned int static_threshold[4];
} vpx_roi_map_t;








typedef struct vpx_active_map {
  unsigned char  *active_map; 
  unsigned int    rows;       
  unsigned int    cols;       
} vpx_active_map_t;






typedef struct vpx_scaling_mode {
  VPX_SCALING_MODE    h_scaling_mode;  
  VPX_SCALING_MODE    v_scaling_mode;  
} vpx_scaling_mode_t;








typedef enum {
  VP8_ONE_TOKENPARTITION   = 0,
  VP8_TWO_TOKENPARTITION   = 1,
  VP8_FOUR_TOKENPARTITION  = 2,
  VP8_EIGHT_TOKENPARTITION = 3
} vp8e_token_partitions;


typedef enum {
  VP9E_CONTENT_DEFAULT,
  VP9E_CONTENT_SCREEN,
  VP9E_CONTENT_INVALID
} vp9e_tune_content;






typedef enum {
  VP8_TUNE_PSNR,
  VP8_TUNE_SSIM
} vp8e_tuning;








typedef struct vpx_svc_layer_id {
  int spatial_layer_id;       
  int temporal_layer_id;      
} vpx_svc_layer_id_t;












VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_UPD_ENTROPY,            int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_UPD_REFERENCE,          int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_USE_REFERENCE,          int)

VPX_CTRL_USE_TYPE(VP8E_SET_FRAME_FLAGS,        int)
VPX_CTRL_USE_TYPE(VP8E_SET_TEMPORAL_LAYER_ID,  int)
VPX_CTRL_USE_TYPE(VP8E_SET_ROI_MAP,            vpx_roi_map_t *)
VPX_CTRL_USE_TYPE(VP8E_SET_ACTIVEMAP,          vpx_active_map_t *)
VPX_CTRL_USE_TYPE(VP8E_SET_SCALEMODE,          vpx_scaling_mode_t *)

VPX_CTRL_USE_TYPE(VP9E_SET_SVC,                int)
VPX_CTRL_USE_TYPE(VP9E_SET_SVC_PARAMETERS,     void *)
VPX_CTRL_USE_TYPE(VP9E_REGISTER_CX_CALLBACK,   void *)
VPX_CTRL_USE_TYPE(VP9E_SET_SVC_LAYER_ID,       vpx_svc_layer_id_t *)

VPX_CTRL_USE_TYPE(VP8E_SET_CPUUSED,            int)
VPX_CTRL_USE_TYPE(VP8E_SET_ENABLEAUTOALTREF,   unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_NOISE_SENSITIVITY,  unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_SHARPNESS,          unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_STATIC_THRESHOLD,   unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_TOKEN_PARTITIONS,   int) 

VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_MAXFRAMES,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_STRENGTH,     unsigned int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_SET_ARNR_TYPE,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_TUNING,             int) 
VPX_CTRL_USE_TYPE(VP8E_SET_CQ_LEVEL,      unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_TILE_COLUMNS,  int)
VPX_CTRL_USE_TYPE(VP9E_SET_TILE_ROWS,  int)

VPX_CTRL_USE_TYPE(VP8E_GET_LAST_QUANTIZER,     int *)
VPX_CTRL_USE_TYPE(VP8E_GET_LAST_QUANTIZER_64,  int *)
VPX_CTRL_USE_TYPE(VP9E_GET_SVC_LAYER_ID,  vpx_svc_layer_id_t *)

VPX_CTRL_USE_TYPE(VP8E_SET_MAX_INTRA_BITRATE_PCT, unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_MAX_INTER_BITRATE_PCT, unsigned int)

VPX_CTRL_USE_TYPE(VP8E_SET_SCREEN_CONTENT_MODE, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_GF_CBR_BOOST_PCT, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_LOSSLESS, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_FRAME_PARALLEL_DECODING, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_AQ_MODE, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_FRAME_PERIODIC_BOOST, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_NOISE_SENSITIVITY,  unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_TUNE_CONTENT, int) 

VPX_CTRL_USE_TYPE(VP9E_SET_COLOR_SPACE, int)

VPX_CTRL_USE_TYPE(VP9E_GET_ACTIVEMAP, vpx_active_map_t *)

#ifdef __cplusplus
}  
#endif

#endif  
