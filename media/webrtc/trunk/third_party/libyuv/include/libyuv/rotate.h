









#ifndef INCLUDE_LIBYUV_ROTATE_H_  
#define INCLUDE_LIBYUV_ROTATE_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


enum RotationMode {
  kRotate0 = 0,  
  kRotate90 = 90,  
  kRotate180 = 180,  
  kRotate270 = 270,  

  
  kRotateNone = 0,
  kRotateClockwise = 90,
  kRotateCounterClockwise = 270,
};


int I420Rotate(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int src_width, int src_height, RotationMode mode);


int NV12ToI420Rotate(const uint8* src_y, int src_stride_y,
                     const uint8* src_uv, int src_stride_uv,
                     uint8* dst_y, int dst_stride_y,
                     uint8* dst_u, int dst_stride_u,
                     uint8* dst_v, int dst_stride_v,
                     int src_width, int src_height, RotationMode mode);

#ifdef __cplusplus
}  
}  
#endif

#endif  
