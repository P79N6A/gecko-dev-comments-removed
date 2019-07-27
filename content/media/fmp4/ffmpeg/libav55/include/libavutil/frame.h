
























#ifndef AVUTIL_FRAME_H
#define AVUTIL_FRAME_H

#include <stdint.h>

#include "avutil.h"
#include "buffer.h"
#include "dict.h"
#include "rational.h"
#include "samplefmt.h"
#include "version.h"










enum AVFrameSideDataType {
    


    AV_FRAME_DATA_PANSCAN,
    




    AV_FRAME_DATA_A53_CC,
    



    AV_FRAME_DATA_STEREO3D,
    


    AV_FRAME_DATA_MATRIXENCODING,
    



    AV_FRAME_DATA_DOWNMIX_INFO,
};

typedef struct AVFrameSideData {
    enum AVFrameSideDataType type;
    uint8_t *data;
    int      size;
    AVDictionary *metadata;
} AVFrameSideData;



























typedef struct AVFrame {
#define AV_NUM_DATA_POINTERS 8
    



    uint8_t *data[AV_NUM_DATA_POINTERS];

    









    int linesize[AV_NUM_DATA_POINTERS];

    













    uint8_t **extended_data;

    


    int width, height;

    


    int nb_samples;

    




    int format;

    


    int key_frame;

    


    enum AVPictureType pict_type;

#if FF_API_AVFRAME_LAVC
    attribute_deprecated
    uint8_t *base[AV_NUM_DATA_POINTERS];
#endif

    


    AVRational sample_aspect_ratio;

    


    int64_t pts;

    


    int64_t pkt_pts;

    


    int64_t pkt_dts;

    


    int coded_picture_number;
    


    int display_picture_number;

    


    int quality;

#if FF_API_AVFRAME_LAVC
    attribute_deprecated
    int reference;

    


    attribute_deprecated
    int8_t *qscale_table;
    


    attribute_deprecated
    int qstride;

    attribute_deprecated
    int qscale_type;

    



    attribute_deprecated
    uint8_t *mbskip_table;

    









    attribute_deprecated
    int16_t (*motion_val[2])[2];

    



    attribute_deprecated
    uint32_t *mb_type;

    


    attribute_deprecated
    short *dct_coeff;

    



    attribute_deprecated
    int8_t *ref_index[2];
#endif

    


    void *opaque;

    


    uint64_t error[AV_NUM_DATA_POINTERS];

#if FF_API_AVFRAME_LAVC
    attribute_deprecated
    int type;
#endif

    



    int repeat_pict;

    


    int interlaced_frame;

    


    int top_field_first;

    


    int palette_has_changed;

#if FF_API_AVFRAME_LAVC
    attribute_deprecated
    int buffer_hints;

    


    attribute_deprecated
    struct AVPanScan *pan_scan;
#endif

    








    int64_t reordered_opaque;

#if FF_API_AVFRAME_LAVC
    


    attribute_deprecated void *hwaccel_picture_private;

    attribute_deprecated
    struct AVCodecContext *owner;
    attribute_deprecated
    void *thread_opaque;

    



    attribute_deprecated
    uint8_t motion_subsample_log2;
#endif

    


    int sample_rate;

    


    uint64_t channel_layout;

    









    AVBufferRef *buf[AV_NUM_DATA_POINTERS];

    











    AVBufferRef **extended_buf;
    


    int        nb_extended_buf;

    AVFrameSideData **side_data;
    int            nb_side_data;











#define AV_FRAME_FLAG_CORRUPT       (1 << 0)




    


    int flags;
} AVFrame;











AVFrame *av_frame_alloc(void);








void av_frame_free(AVFrame **frame);












int av_frame_ref(AVFrame *dst, const AVFrame *src);








AVFrame *av_frame_clone(const AVFrame *src);




void av_frame_unref(AVFrame *frame);




void av_frame_move_ref(AVFrame *dst, AVFrame *src);


















int av_frame_get_buffer(AVFrame *frame, int align);













int av_frame_is_writable(AVFrame *frame);












int av_frame_make_writable(AVFrame *frame);









int av_frame_copy_props(AVFrame *dst, const AVFrame *src);









AVBufferRef *av_frame_get_plane_buffer(AVFrame *frame, int plane);










AVFrameSideData *av_frame_new_side_data(AVFrame *frame,
                                        enum AVFrameSideDataType type,
                                        int size);





AVFrameSideData *av_frame_get_side_data(const AVFrame *frame,
                                        enum AVFrameSideDataType type);





#endif 
