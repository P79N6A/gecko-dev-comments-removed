

























#ifdef __cplusplus
extern "C" {
#endif

#ifndef VPX_DECODER_H
#define VPX_DECODER_H
#include "vpx_codec.h"

    







#define VPX_DECODER_ABI_VERSION (2 + VPX_CODEC_ABI_VERSION) /**<\hideinitializer*/

    







#define VPX_CODEC_CAP_PUT_SLICE  0x10000 /**< Will issue put_slice callbacks */
#define VPX_CODEC_CAP_PUT_FRAME  0x20000 /**< Will issue put_frame callbacks */
#define VPX_CODEC_CAP_POSTPROC   0x40000 /**< Can postprocess decoded frame */
#define VPX_CODEC_CAP_ERROR_CONCEALMENT   0x80000 /**< Can conceal errors due to
                                                       packet loss */
#define VPX_CODEC_CAP_INPUT_PARTITION   0x100000 /**< Can receive encoded frames
                                                    one partition at a time */

    






#define VPX_CODEC_USE_POSTPROC   0x10000 /**< Postprocess decoded frame */
#define VPX_CODEC_USE_ERROR_CONCEALMENT 0x20000 /**< Conceal errors in decoded
                                                     frames */
#define VPX_CODEC_USE_INPUT_PARTITION   0x40000 /**< The input frame should be
                                                    passed to the decoder one
                                                    partition at a time */

    





    typedef struct vpx_codec_stream_info
    {
        unsigned int sz;     
        unsigned int w;      
        unsigned int h;      
        unsigned int is_kf;  
    } vpx_codec_stream_info_t;

    






    




    typedef struct vpx_codec_dec_cfg
    {
        unsigned int threads; 
        unsigned int w;      
        unsigned int h;      
    } vpx_codec_dec_cfg_t; 


    





















    vpx_codec_err_t vpx_codec_dec_init_ver(vpx_codec_ctx_t      *ctx,
                                           vpx_codec_iface_t    *iface,
                                           vpx_codec_dec_cfg_t  *cfg,
                                           vpx_codec_flags_t     flags,
                                           int                   ver);

    



#define vpx_codec_dec_init(ctx, iface, cfg, flags) \
    vpx_codec_dec_init_ver(ctx, iface, cfg, flags, VPX_DECODER_ABI_VERSION)


    
















    vpx_codec_err_t vpx_codec_peek_stream_info(vpx_codec_iface_t       *iface,
            const uint8_t           *data,
            unsigned int             data_sz,
            vpx_codec_stream_info_t *si);


    












    vpx_codec_err_t vpx_codec_get_stream_info(vpx_codec_ctx_t         *ctx,
            vpx_codec_stream_info_t *si);


    



























    vpx_codec_err_t vpx_codec_decode(vpx_codec_ctx_t    *ctx,
                                     const uint8_t        *data,
                                     unsigned int            data_sz,
                                     void               *user_priv,
                                     long                deadline);


    














    vpx_image_t *vpx_codec_get_frame(vpx_codec_ctx_t  *ctx,
                                     vpx_codec_iter_t *iter);


    








    




    typedef void (*vpx_codec_put_frame_cb_fn_t)(void        *user_priv,
            const vpx_image_t *img);


    














    vpx_codec_err_t vpx_codec_register_put_frame_cb(vpx_codec_ctx_t             *ctx,
            vpx_codec_put_frame_cb_fn_t  cb,
            void                        *user_priv);


    

    








    




    typedef void (*vpx_codec_put_slice_cb_fn_t)(void         *user_priv,
            const vpx_image_t      *img,
            const vpx_image_rect_t *valid,
            const vpx_image_rect_t *update);


    














    vpx_codec_err_t vpx_codec_register_put_slice_cb(vpx_codec_ctx_t             *ctx,
            vpx_codec_put_slice_cb_fn_t  cb,
            void                        *user_priv);


    

    

#endif

#ifdef __cplusplus
}
#endif

#if !defined(VPX_CODEC_DISABLE_COMPAT) || !VPX_CODEC_DISABLE_COMPAT
#include "vpx_decoder_compat.h"
#endif
