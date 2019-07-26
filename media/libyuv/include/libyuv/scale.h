









#ifndef INCLUDE_LIBYUV_SCALE_H_  
#define INCLUDE_LIBYUV_SCALE_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


typedef enum FilterMode {
  kFilterNone = 0,  
  kFilterLinear = 1,  
  kFilterBilinear = 2,  
  kFilterBox = 3  
} FilterModeEnum;


LIBYUV_API
void ScalePlane(const uint8* src, int src_stride,
                int src_width, int src_height,
                uint8* dst, int dst_stride,
                int dst_width, int dst_height,
                enum FilterMode filtering);











LIBYUV_API
int I420Scale(const uint8* src_y, int src_stride_y,
              const uint8* src_u, int src_stride_u,
              const uint8* src_v, int src_stride_v,
              int src_width, int src_height,
              uint8* dst_y, int dst_stride_y,
              uint8* dst_u, int dst_stride_u,
              uint8* dst_v, int dst_stride_v,
              int dst_width, int dst_height,
              enum FilterMode filtering);

#ifdef __cplusplus

LIBYUV_API
int Scale(const uint8* src_y, const uint8* src_u, const uint8* src_v,
          int src_stride_y, int src_stride_u, int src_stride_v,
          int src_width, int src_height,
          uint8* dst_y, uint8* dst_u, uint8* dst_v,
          int dst_stride_y, int dst_stride_u, int dst_stride_v,
          int dst_width, int dst_height,
          LIBYUV_BOOL interpolate);


LIBYUV_API
int ScaleOffset(const uint8* src_i420, int src_width, int src_height,
                uint8* dst_i420, int dst_width, int dst_height, int dst_yoffset,
                LIBYUV_BOOL interpolate);


LIBYUV_API
void SetUseReferenceImpl(LIBYUV_BOOL use);
#endif  

#ifdef __cplusplus
}  
}  
#endif

#endif  
