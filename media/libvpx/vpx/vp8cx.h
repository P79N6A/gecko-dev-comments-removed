















#include "vp8.h"





#ifndef VP8CX_H
#define VP8CX_H
#include "vpx_codec_impl_top.h"







extern vpx_codec_iface_t  vpx_codec_vp8_cx_algo;
extern vpx_codec_iface_t* vpx_codec_vp8_cx(void);













#define VP8_EFLAG_NO_REF_LAST      (1<<16)








#define VP8_EFLAG_NO_REF_GF        (1<<17)








#define VP8_EFLAG_NO_REF_ARF       (1<<21)







#define VP8_EFLAG_NO_UPD_LAST      (1<<18)







#define VP8_EFLAG_NO_UPD_GF        (1<<22)







#define VP8_EFLAG_NO_UPD_ARF       (1<<23)







#define VP8_EFLAG_FORCE_GF         (1<<19)







#define VP8_EFLAG_FORCE_ARF        (1<<24)







#define VP8_EFLAG_NO_UPD_ENTROPY   (1<<20)









enum vp8e_enc_control_id
{
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
    VP8E_SET_ARNR_STRENGTH ,         
    VP8E_SET_ARNR_TYPE     ,         
    VP8E_SET_TUNING,                 
    





    VP8E_SET_CQ_LEVEL,

    











    VP8E_SET_MAX_INTRA_BITRATE_PCT,
};





typedef enum vpx_scaling_mode_1d
{
    VP8E_NORMAL      = 0,
    VP8E_FOURFIVE    = 1,
    VP8E_THREEFIVE   = 2,
    VP8E_ONETWO      = 3
} VPX_SCALING_MODE;








typedef struct vpx_roi_map
{
    unsigned char *roi_map;      
    unsigned int   rows;         
    unsigned int   cols;         
    int     delta_q[4];          
    int     delta_lf[4];         
    unsigned int   static_threshold[4];
} vpx_roi_map_t;








typedef struct vpx_active_map
{
    unsigned char  *active_map; 
    unsigned int    rows;       
    unsigned int    cols;       
} vpx_active_map_t;






typedef struct vpx_scaling_mode
{
    VPX_SCALING_MODE    h_scaling_mode;  
    VPX_SCALING_MODE    v_scaling_mode;  
} vpx_scaling_mode_t;






typedef enum
{
    VP8_BEST_QUALITY_ENCODING,
    VP8_GOOD_QUALITY_ENCODING,
    VP8_REAL_TIME_ENCODING
} vp8e_encoding_mode;








typedef enum
{
    VP8_ONE_TOKENPARTITION   = 0,
    VP8_TWO_TOKENPARTITION   = 1,
    VP8_FOUR_TOKENPARTITION  = 2,
    VP8_EIGHT_TOKENPARTITION = 3,
} vp8e_token_partitions;







typedef enum
{
    VP8_TUNE_PSNR,
    VP8_TUNE_SSIM
} vp8e_tuning;













VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_UPD_ENTROPY,            int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_UPD_REFERENCE,          int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_USE_REFERENCE,          int)

VPX_CTRL_USE_TYPE(VP8E_SET_ROI_MAP,            vpx_roi_map_t *)
VPX_CTRL_USE_TYPE(VP8E_SET_ACTIVEMAP,          vpx_active_map_t *)
VPX_CTRL_USE_TYPE(VP8E_SET_SCALEMODE,          vpx_scaling_mode_t *)

VPX_CTRL_USE_TYPE(VP8E_SET_CPUUSED,            int)
VPX_CTRL_USE_TYPE(VP8E_SET_ENABLEAUTOALTREF,   unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_NOISE_SENSITIVITY,  unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_SHARPNESS,          unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_STATIC_THRESHOLD,   unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_TOKEN_PARTITIONS,   vp8e_token_partitions)

VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_MAXFRAMES,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_STRENGTH ,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_TYPE     ,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_TUNING,             vp8e_tuning)
VPX_CTRL_USE_TYPE(VP8E_SET_CQ_LEVEL     ,      unsigned int)

VPX_CTRL_USE_TYPE(VP8E_GET_LAST_QUANTIZER,     int *)
VPX_CTRL_USE_TYPE(VP8E_GET_LAST_QUANTIZER_64,  int *)

VPX_CTRL_USE_TYPE(VP8E_SET_MAX_INTRA_BITRATE_PCT, unsigned int)



#include "vpx_codec_impl_bottom.h"
#endif
