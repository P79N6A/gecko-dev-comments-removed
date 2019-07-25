





























#ifndef VP8_H
#define VP8_H
#include "vpx_codec_impl_top.h"





enum vp8_com_control_id
{
    VP8_SET_REFERENCE           = 1,    
    VP8_COPY_REFERENCE          = 2,    
    VP8_SET_POSTPROC            = 3,    
    VP8_SET_DBG_COLOR_REF_FRAME = 4,    
    VP8_SET_DBG_COLOR_MB_MODES  = 5,    
    VP8_SET_DBG_COLOR_B_MODES   = 6,    
    VP8_SET_DBG_DISPLAY_MV      = 7,    
    VP8_COMMON_CTRL_ID_MAX,
    VP8_DECODER_CTRL_ID_START   = 256
};





enum vp8_postproc_level
{
    VP8_NOFILTERING             = 0,
    VP8_DEBLOCK                 = 1<<0,
    VP8_DEMACROBLOCK            = 1<<1,
    VP8_ADDNOISE                = 1<<2,
    VP8_DEBUG_TXT_FRAME_INFO    = 1<<3, 
    VP8_DEBUG_TXT_MBLK_MODES    = 1<<4, 
    VP8_DEBUG_TXT_DC_DIFF       = 1<<5, 
    VP8_DEBUG_TXT_RATE_INFO     = 1<<6  
};








typedef struct vp8_postproc_cfg
{
    int post_proc_flag;         
    int deblocking_level;       
    int noise_level;            
} vp8_postproc_cfg_t;





typedef enum vpx_ref_frame_type
{
    VP8_LAST_FRAME = 1,
    VP8_GOLD_FRAME = 2,
    VP8_ALTR_FRAME = 4
} vpx_ref_frame_type_t;






typedef struct vpx_ref_frame
{
    vpx_ref_frame_type_t  frame_type;   
    vpx_image_t           img;          
} vpx_ref_frame_t;







VPX_CTRL_USE_TYPE(VP8_SET_REFERENCE,           vpx_ref_frame_t *)
VPX_CTRL_USE_TYPE(VP8_COPY_REFERENCE,          vpx_ref_frame_t *)
VPX_CTRL_USE_TYPE(VP8_SET_POSTPROC,            vp8_postproc_cfg_t *)
VPX_CTRL_USE_TYPE(VP8_SET_DBG_COLOR_REF_FRAME, int)
VPX_CTRL_USE_TYPE(VP8_SET_DBG_COLOR_MB_MODES,  int)
VPX_CTRL_USE_TYPE(VP8_SET_DBG_COLOR_B_MODES,   int)
VPX_CTRL_USE_TYPE(VP8_SET_DBG_DISPLAY_MV,      int)




#if !defined(VPX_CODEC_DISABLE_COMPAT) || !VPX_CODEC_DISABLE_COMPAT




DECLSPEC_DEPRECATED extern vpx_codec_iface_t vpx_codec_vp8_algo DEPRECATED;
#endif

#include "vpx_codec_impl_bottom.h"
#endif
