









#ifndef VP9_ENCODER_VP9_FIRSTPASS_H_
#define VP9_ENCODER_VP9_FIRSTPASS_H_
#include "vp9/encoder/vp9_onyx_int.h"

void vp9_init_first_pass(VP9_COMP *cpi);
void vp9_first_pass(VP9_COMP *cpi);
void vp9_end_first_pass(VP9_COMP *cpi);

void vp9_init_second_pass(VP9_COMP *cpi);
void vp9_second_pass(VP9_COMP *cpi);
void vp9_end_second_pass(VP9_COMP *cpi);

#endif  
