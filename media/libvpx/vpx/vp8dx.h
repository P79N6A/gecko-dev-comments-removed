










#include "vp8.h"










#ifndef VP8DX_H
#define VP8DX_H
#include "vpx_codec_impl_top.h"







extern vpx_codec_iface_t  vpx_codec_vp8_dx_algo;
extern vpx_codec_iface_t* vpx_codec_vp8_dx(void);



#include "vp8.h"









enum vp8_dec_control_id
{
    


    VP8D_GET_LAST_REF_UPDATES = VP8_DECODER_CTRL_ID_START,

    
    VP8D_GET_FRAME_CORRUPTED,

    VP8_DECODER_CTRL_ID_MAX
} ;










VPX_CTRL_USE_TYPE(VP8D_GET_LAST_REF_UPDATES,   int *)
VPX_CTRL_USE_TYPE(VP8D_GET_FRAME_CORRUPTED,    int *)





#include "vpx_codec_impl_bottom.h"
#endif
