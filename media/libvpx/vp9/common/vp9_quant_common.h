









#ifndef VP9_COMMON_VP9_QUANT_COMMON_H_
#define VP9_COMMON_VP9_QUANT_COMMON_H_

#include "vp9/common/vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MINQ 0
#define MAXQ 255
#define QINDEX_RANGE (MAXQ - MINQ + 1)
#define QINDEX_BITS 8

void vp9_init_quant_tables();

int16_t vp9_dc_quant(int qindex, int delta);
int16_t vp9_ac_quant(int qindex, int delta);

int vp9_get_qindex(const struct segmentation *seg, int segment_id,
                   int base_qindex);

#ifdef __cplusplus
}  
#endif

#endif
