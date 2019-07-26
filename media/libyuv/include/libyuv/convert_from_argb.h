









#ifndef INCLUDE_LIBYUV_CONVERT_FROM_ARGB_H_  
#define INCLUDE_LIBYUV_CONVERT_FROM_ARGB_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#define ARGBToARGB ARGBCopy
LIBYUV_API
int ARGBCopy(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int width, int height);


#define ARGBToBGRA BGRAToARGB
LIBYUV_API
int BGRAToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


#define ARGBToABGR ABGRToARGB
LIBYUV_API
int ABGRToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int ARGBToRGBA(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int ARGBToRGB24(const uint8* src_argb, int src_stride_argb,
                uint8* dst_rgb24, int dst_stride_rgb24,
                int width, int height);


LIBYUV_API
int ARGBToRAW(const uint8* src_argb, int src_stride_argb,
              uint8* dst_rgb, int dst_stride_rgb,
              int width, int height);


LIBYUV_API
int ARGBToRGB565(const uint8* src_argb, int src_stride_argb,
                 uint8* dst_rgb565, int dst_stride_rgb565,
                 int width, int height);


LIBYUV_API
int ARGBToARGB1555(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_argb1555, int dst_stride_argb1555,
                   int width, int height);


LIBYUV_API
int ARGBToARGB4444(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_argb4444, int dst_stride_argb4444,
                   int width, int height);


LIBYUV_API
int ARGBToI444(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int ARGBToI422(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int ARGBToI420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int ARGBToJ420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yj, int dst_stride_yj,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int ARGBToI411(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int ARGBToJ400(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yj, int dst_stride_yj,
               int width, int height);


LIBYUV_API
int ARGBToI400(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               int width, int height);


LIBYUV_API
int ARGBToNV12(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_uv, int dst_stride_uv,
               int width, int height);


LIBYUV_API
int ARGBToNV21(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_vu, int dst_stride_vu,
               int width, int height);


LIBYUV_API
int ARGBToNV21(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_vu, int dst_stride_vu,
               int width, int height);


LIBYUV_API
int ARGBToYUY2(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yuy2, int dst_stride_yuy2,
               int width, int height);


LIBYUV_API
int ARGBToUYVY(const uint8* src_argb, int src_stride_argb,
               uint8* dst_uyvy, int dst_stride_uyvy,
               int width, int height);

#ifdef __cplusplus
}  
}  
#endif

#endif  
