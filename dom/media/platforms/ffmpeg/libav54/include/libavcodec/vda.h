





















#ifndef AVCODEC_VDA_H
#define AVCODEC_VDA_H







#include "libavcodec/version.h"

#if FF_API_VDA_ASYNC
#include <pthread.h>
#endif

#include <stdint.h>



#undef __GNUC_STDC_INLINE__

#define Picture QuickdrawPicture
#include <VideoDecodeAcceleration/VDADecoder.h>
#undef Picture








#if FF_API_VDA_ASYNC





typedef struct vda_frame {
    





    int64_t             pts;

    





    CVPixelBufferRef    cv_buffer;

    





    struct vda_frame    *next_frame;
} vda_frame;
#endif







struct vda_context {
    





    VDADecoder          decoder;

    





    CVPixelBufferRef    cv_buffer;

    





    int                 use_sync_decoding;

#if FF_API_VDA_ASYNC
    







    vda_frame           *queue;

    







    pthread_mutex_t     queue_mutex;
#endif

    





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

#if FF_API_VDA_ASYNC





vda_frame *ff_vda_queue_pop(struct vda_context *vda_ctx);






void ff_vda_release_vda_frame(vda_frame *frame);
#endif





#endif 
