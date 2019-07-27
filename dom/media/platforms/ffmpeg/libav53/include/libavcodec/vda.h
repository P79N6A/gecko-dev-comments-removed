





















#ifndef AVCODEC_VDA_H
#define AVCODEC_VDA_H

#include <pthread.h>
#include <stdint.h>



#undef __GNUC_STDC_INLINE__

#define Picture QuickdrawPicture
#include <VideoDecodeAcceleration/VDADecoder.h>
#undef Picture




typedef struct vda_frame {
    





    int64_t             pts;

    





    CVPixelBufferRef    cv_buffer;

    





    struct vda_frame    *next_frame;
} vda_frame;







struct vda_context {
    





    VDADecoder          decoder;

    





    vda_frame           *queue;

    





    pthread_mutex_t     queue_mutex;

    





    int                 width;

    





    int                 height;

    





    int                 format;

    





    OSType              cv_pix_fmt_type;
};


int ff_vda_create_decoder(struct vda_context *vda_ctx,
                          uint8_t *extradata,
                          int extradata_size);


int ff_vda_destroy_decoder(struct vda_context *vda_ctx);


vda_frame *ff_vda_queue_pop(struct vda_context *vda_ctx);


void ff_vda_release_vda_frame(vda_frame *frame);

#endif 
