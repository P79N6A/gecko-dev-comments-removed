









#ifndef VP8_COMMON_QUANT_COMMON_H_
#define VP8_COMMON_QUANT_COMMON_H_


#include "string.h"
#include "blockd.h"
#include "onyxc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int vp8_ac_yquant(int QIndex);
extern int vp8_dc_quant(int QIndex, int Delta);
extern int vp8_dc2quant(int QIndex, int Delta);
extern int vp8_ac2quant(int QIndex, int Delta);
extern int vp8_dc_uv_quant(int QIndex, int Delta);
extern int vp8_ac_uv_quant(int QIndex, int Delta);

#ifdef __cplusplus
}  
#endif

#endif
