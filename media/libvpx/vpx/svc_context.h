














#ifndef VPX_SVC_CONTEXT_H_
#define VPX_SVC_CONTEXT_H_

#include "./vp8cx.h"
#include "./vpx_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SVC_LOG_LEVEL {
  SVC_LOG_ERROR,
  SVC_LOG_INFO,
  SVC_LOG_DEBUG
} SVC_LOG_LEVEL;

typedef struct {
  
  int spatial_layers;               
  int temporal_layers;               
  SVC_LOG_LEVEL log_level;  
  int log_print;  
                  

  
  void *internal;
} SvcContext;









vpx_codec_err_t vpx_svc_set_options(SvcContext *svc_ctx, const char *options);




vpx_codec_err_t vpx_svc_init(SvcContext *svc_ctx, vpx_codec_ctx_t *codec_ctx,
                             vpx_codec_iface_t *iface,
                             vpx_codec_enc_cfg_t *cfg);



vpx_codec_err_t vpx_svc_encode(SvcContext *svc_ctx, vpx_codec_ctx_t *codec_ctx,
                               struct vpx_image *rawimg, vpx_codec_pts_t pts,
                               int64_t duration, int deadline);




void vpx_svc_release(SvcContext *svc_ctx);




const char *vpx_svc_dump_statistics(SvcContext *svc_ctx);




const char *vpx_svc_get_message(const SvcContext *svc_ctx);

#ifdef __cplusplus
}  
#endif

#endif
