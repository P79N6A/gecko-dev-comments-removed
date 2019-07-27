









#ifndef VP9_ENCODER_VP9_TEMPORAL_FILTER_H_
#define VP9_ENCODER_VP9_TEMPORAL_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

void vp9_temporal_filter_prepare(VP9_COMP *cpi, int distance);
void vp9_configure_arnr_filter(VP9_COMP *cpi,
                               const unsigned int frames_to_arnr,
                               const int group_boost);

#ifdef __cplusplus
}  
#endif

#endif
