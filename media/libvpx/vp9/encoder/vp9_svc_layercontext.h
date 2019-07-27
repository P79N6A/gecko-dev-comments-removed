









#ifndef VP9_ENCODER_VP9_SVC_LAYERCONTEXT_H_
#define VP9_ENCODER_VP9_SVC_LAYERCONTEXT_H_

#include "vpx/vpx_encoder.h"

#include "vp9/encoder/vp9_ratectrl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  RATE_CONTROL rc;
  int target_bandwidth;
  double framerate;
  int avg_frame_size;
  TWO_PASS twopass;
  vpx_fixed_buf_t rc_twopass_stats_in;
  unsigned int current_video_frame_in_layer;
  int is_key_frame;
  int frames_from_key_frame;
  FRAME_TYPE last_frame_type;
  vpx_svc_parameters_t svc_params_received;
  struct lookahead_entry  *alt_ref_source;
  int alt_ref_idx;
  int gold_ref_idx;
  int has_alt_frame;
  size_t layer_size;
} LAYER_CONTEXT;

typedef struct {
  int spatial_layer_id;
  int temporal_layer_id;
  int number_spatial_layers;
  int number_temporal_layers;

  
  
  YV12_BUFFER_CONFIG scaled_frames[MAX_LAG_BUFFERS];

  
  
  
  LAYER_CONTEXT layer_context[MAX(VPX_TS_MAX_LAYERS, VPX_SS_MAX_LAYERS)];
} SVC;

struct VP9_COMP;


void vp9_init_layer_context(struct VP9_COMP *const cpi);


void vp9_update_layer_context_change_config(struct VP9_COMP *const cpi,
                                            const int target_bandwidth);



void vp9_update_temporal_layer_framerate(struct VP9_COMP *const cpi);


void vp9_update_spatial_layer_framerate(struct VP9_COMP *const cpi,
                                        double framerate);



void vp9_restore_layer_context(struct VP9_COMP *const cpi);


void vp9_save_layer_context(struct VP9_COMP *const cpi);


void vp9_init_second_pass_spatial_svc(struct VP9_COMP *cpi);


void vp9_inc_frame_in_layer(struct VP9_COMP *const cpi);


int vp9_is_upper_layer_key_frame(const struct VP9_COMP *const cpi);



int vp9_svc_lookahead_push(const struct VP9_COMP *const cpi,
                           struct lookahead_ctx *ctx, YV12_BUFFER_CONFIG *src,
                           int64_t ts_start, int64_t ts_end,
                           unsigned int flags);


struct lookahead_entry *vp9_svc_lookahead_pop(struct VP9_COMP *const cpi,
                                              struct lookahead_ctx *ctx,
                                              int drain);


struct lookahead_entry *vp9_svc_lookahead_peek(struct VP9_COMP *const cpi,
                                               struct lookahead_ctx *ctx,
                                               int index, int copy_params);

#ifdef __cplusplus
}  
#endif

#endif
