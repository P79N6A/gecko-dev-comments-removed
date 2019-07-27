










#ifndef VP9_ENCODER_VP9_PICKLPF_H_
#define VP9_ENCODER_VP9_PICKLPF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vp9/encoder/vp9_encoder.h"

struct yv12_buffer_config;
struct VP9_COMP;

void vp9_pick_filter_level(const struct yv12_buffer_config *sd,
                           struct VP9_COMP *cpi, LPF_PICK_METHOD method);
#ifdef __cplusplus
}  
#endif

#endif
