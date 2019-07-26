









#ifndef VP9_ENCODER_VP9_TEMPORAL_FILTER_H_
#define VP9_ENCODER_VP9_TEMPORAL_FILTER_H_

void vp9_temporal_filter_prepare(VP9_COMP *cpi, int distance);
void configure_arnr_filter(VP9_COMP *cpi, const unsigned int this_frame,
                           const int group_boost);

#endif  
