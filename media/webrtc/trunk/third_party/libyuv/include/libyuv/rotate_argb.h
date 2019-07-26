









#ifndef INCLUDE_LIBYUV_ROTATE_ARGB_H_  
#define INCLUDE_LIBYUV_ROTATE_ARGB_H_

#include "libyuv/basic_types.h"
#include "libyuv/rotate.h"  

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


LIBYUV_API
int ARGBRotate(const uint8* src_argb, int src_stride_argb,
               uint8* dst_argb, int dst_stride_argb,
               int src_width, int src_height, enum RotationMode mode);

#ifdef __cplusplus
}  
}  
#endif

#endif  
