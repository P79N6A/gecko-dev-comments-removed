









#ifndef VP8_COMMON_X86_FILTER_X86_H_
#define VP8_COMMON_X86_FILTER_X86_H_

#include "vpx_ports/mem.h"

#ifdef __cplusplus
extern "C" {
#endif





extern DECLARE_ALIGNED(16, const short, vp8_bilinear_filters_x86_4[8][8]);


extern DECLARE_ALIGNED(16, const short, vp8_bilinear_filters_x86_8[8][16]);

#ifdef __cplusplus
}  
#endif

#endif
