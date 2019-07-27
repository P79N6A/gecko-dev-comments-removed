









#ifndef VP9_ENCODER_VP9_TEMPORAL_FILTER_H_
#define VP9_ENCODER_VP9_TEMPORAL_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

void vp9_temporal_filter_init();
void vp9_temporal_filter(VP9_COMP *cpi, int distance);

#ifdef __cplusplus
}  
#endif

#endif
