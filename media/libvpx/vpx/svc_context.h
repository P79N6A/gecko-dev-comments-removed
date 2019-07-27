














#ifndef VPX_SVC_CONTEXT_H_
#define VPX_SVC_CONTEXT_H_

#include "./vp8cx.h"
#include "./vpx_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SVC_ENCODING_MODE {
  INTER_LAYER_PREDICTION_I,
  ALT_INTER_LAYER_PREDICTION_IP,
  INTER_LAYER_PREDICTION_IP,
  USE_GOLDEN_FRAME
} SVC_ENCODING_MODE;

typedef enum SVC_LOG_LEVEL {
  SVC_LOG_ERROR,
  SVC_LOG_INFO,
  SVC_LOG_DEBUG
} SVC_LOG_LEVEL;

typedef struct {
  
  int spatial_layers;               
  SVC_ENCODING_MODE encoding_mode;  
  SVC_LOG_LEVEL log_level;  
  int log_print;  
                  

  
  void *internal;
} SvcContext;









vpx_codec_err_t vpx_svc_set_options(SvcContext *svc_ctx, const char *options);






vpx_codec_err_t vpx_svc_set_quantizers(SvcContext *svc_ctx,
                                       const char *quantizer_values);






vpx_codec_err_t vpx_svc_set_scale_factors(SvcContext *svc_ctx,
                                          const char *scale_factors);




vpx_codec_err_t vpx_svc_init(SvcContext *svc_ctx, vpx_codec_ctx_t *codec_ctx,
                             vpx_codec_iface_t *iface,
                             vpx_codec_enc_cfg_t *cfg);



vpx_codec_err_t vpx_svc_encode(SvcContext *svc_ctx, vpx_codec_ctx_t *codec_ctx,
                               struct vpx_image *rawimg, vpx_codec_pts_t pts,
                               int64_t duration, int deadline);




void vpx_svc_release(SvcContext *svc_ctx);




const char *vpx_svc_dump_statistics(SvcContext *svc_ctx);




const char *vpx_svc_get_message(const SvcContext *svc_ctx);




size_t vpx_svc_get_frame_size(const SvcContext *svc_ctx);




void *vpx_svc_get_buffer(const SvcContext *svc_ctx);




vpx_codec_err_t vpx_svc_get_layer_resolution(const SvcContext *svc_ctx,
                                             int layer,
                                             unsigned int *width,
                                             unsigned int *height);



int vpx_svc_get_encode_frame_count(const SvcContext *svc_ctx);




int vpx_svc_is_keyframe(const SvcContext *svc_ctx);




void vpx_svc_set_keyframe(SvcContext *svc_ctx);

#ifdef __cplusplus
}  
#endif

#endif
