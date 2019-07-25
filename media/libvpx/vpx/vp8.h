





























#ifndef VP8_H
#define VP8_H
#include "vpx/vpx_codec_impl_top.h"





enum vp8_dec_control_id
{
    VP8_SET_REFERENCE       = 1,    
    VP8_COPY_REFERENCE      = 2,    
    VP8_SET_POSTPROC        = 3,    
    VP8_COMMON_CTRL_ID_MAX
};





enum vp8_postproc_level
{
    VP8_NOFILTERING    = 0,
    VP8_DEBLOCK        = 1,
    VP8_DEMACROBLOCK   = 2,
    VP8_ADDNOISE       = 4
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




#if !defined(VPX_CODEC_DISABLE_COMPAT) || !VPX_CODEC_DISABLE_COMPAT




DECLSPEC_DEPRECATED extern vpx_codec_iface_t vpx_codec_vp8_algo DEPRECATED;
#endif

#include "vpx/vpx_codec_impl_bottom.h"
#endif
