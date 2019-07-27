









#ifndef VP9_ENCODER_VP9_SKIN_MAP_H_
#define VP9_ENCODER_VP9_SKIN_MAP_H_

#include "vp9/common/vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VP9_COMP;



int vp9_skin_pixel(const uint8_t y, const uint8_t cb, const uint8_t cr);

#ifdef OUTPUT_YUV_SKINMAP

void vp9_compute_skin_map(VP9_COMP *const cpi, FILE *yuv_skinmap_file);
#endif

#ifdef __cplusplus
}  
#endif

#endif
