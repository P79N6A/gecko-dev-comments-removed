









#ifndef INCLUDE_LIBYUV_SCALE_ARGB_H_  
#define INCLUDE_LIBYUV_SCALE_ARGB_H_

#include "libyuv/basic_types.h"
#include "libyuv/scale.h"  

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

LIBYUV_API
int ARGBScale(const uint8* src_argb, int src_stride_argb,
              int src_width, int src_height,
              uint8* dst_argb, int dst_stride_argb,
              int dst_width, int dst_height,
              FilterMode filtering);

#ifdef __cplusplus
}  
}  
#endif

#endif  
