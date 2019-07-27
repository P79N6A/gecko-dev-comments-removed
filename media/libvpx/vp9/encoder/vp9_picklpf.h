










#ifndef VP9_ENCODER_VP9_PICKLPF_H_
#define VP9_ENCODER_VP9_PICKLPF_H_

#ifdef __cplusplus
extern "C" {
#endif

struct yv12_buffer_config;
struct VP9_COMP;

void vp9_set_alt_lf_level(struct VP9_COMP *cpi, int filt_val);

void vp9_pick_filter_level(const struct yv12_buffer_config *sd,
                           struct VP9_COMP *cpi, int method);
#ifdef __cplusplus
}  
#endif

#endif
