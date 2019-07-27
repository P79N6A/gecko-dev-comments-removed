






















#ifndef AVCODEC_VDPAU_H
#define AVCODEC_VDPAU_H


























#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

#include "libavutil/attributes.h"

#include "avcodec.h"
#include "version.h"

#if FF_API_BUFS_VDPAU
union AVVDPAUPictureInfo {
    VdpPictureInfoH264        h264;
    VdpPictureInfoMPEG1Or2    mpeg;
    VdpPictureInfoVC1          vc1;
    VdpPictureInfoMPEG4Part2 mpeg4;
};
#endif














typedef struct AVVDPAUContext {
    




    VdpDecoder decoder;

    




    VdpDecoderRender *render;

#if FF_API_BUFS_VDPAU
    




    attribute_deprecated
    union AVVDPAUPictureInfo info;

    




    attribute_deprecated
    int bitstream_buffers_allocated;

    




    attribute_deprecated
    int bitstream_buffers_used;

   





    attribute_deprecated
    VdpBitstreamBuffer *bitstream_buffers;
#endif
} AVVDPAUContext;






AVVDPAUContext *av_vdpau_alloc_context(void);












int av_vdpau_get_profile(AVCodecContext *avctx, VdpDecoderProfile *profile);

#if FF_API_CAP_VDPAU

#define FF_VDPAU_STATE_USED_FOR_RENDER 1





#define FF_VDPAU_STATE_USED_FOR_REFERENCE 2








struct vdpau_render_state {
    VdpVideoSurface surface; 

    int state; 

    
    union AVVDPAUPictureInfo info;

    

    int bitstream_buffers_allocated;
    int bitstream_buffers_used;
    
    VdpBitstreamBuffer *bitstream_buffers;
};
#endif



#endif 
