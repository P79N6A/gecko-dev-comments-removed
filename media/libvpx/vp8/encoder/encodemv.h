










#ifndef VP8_ENCODER_ENCODEMV_H_
#define VP8_ENCODER_ENCODEMV_H_

#include "onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp8_write_mvprobs(VP8_COMP *);
void vp8_encode_motion_vector(vp8_writer *, const MV *, const MV_CONTEXT *);
void vp8_build_component_cost_table(int *mvcost[2], const MV_CONTEXT *mvc, int mvc_flag[2]);

#ifdef __cplusplus
}  
#endif

#endif
