





















#ifndef AVCODEC_VDA_H
#define AVCODEC_VDA_H







#include "libavcodec/version.h"

#include <stdint.h>



#undef __GNUC_STDC_INLINE__

#define Picture QuickdrawPicture
#include <VideoDecodeAcceleration/VDADecoder.h>
#undef Picture














struct vda_context {
    





    VDADecoder          decoder;

    





    CVPixelBufferRef    cv_buffer;

    





    int                 use_sync_decoding;

    





    int                 width;

    





    int                 height;

    





    int                 format;

    





    OSType              cv_pix_fmt_type;

    


    uint8_t             *priv_bitstream;

    


    int                 priv_bitstream_size;

    


    int                 priv_allocated_size;
};


int ff_vda_create_decoder(struct vda_context *vda_ctx,
                          uint8_t *extradata,
                          int extradata_size);


int ff_vda_destroy_decoder(struct vda_context *vda_ctx);





#endif 
