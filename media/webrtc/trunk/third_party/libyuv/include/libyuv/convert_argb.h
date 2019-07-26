









#ifndef INCLUDE_LIBYUV_CONVERT_ARGB_H_  
#define INCLUDE_LIBYUV_CONVERT_ARGB_H_

#include "libyuv/basic_types.h"

#include "libyuv/convert_from.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"







#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#define ARGBToARGB ARGBCopy


LIBYUV_API
int ARGBCopy(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int width, int height);


LIBYUV_API
int I420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int I422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int I444ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int I411ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int I400ToARGB(const uint8* src_y, int src_stride_y,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int I400ToARGB_Reference(const uint8* src_y, int src_stride_y,
                         uint8* dst_argb, int dst_stride_argb,
                         int width, int height);


LIBYUV_API
int NV12ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int NV21ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_vu, int src_stride_vu,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int M420ToARGB(const uint8* src_m420, int src_stride_m420,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);









LIBYUV_API
int YUY2ToARGB(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int UYVYToARGB(const uint8* src_uyvy, int src_stride_uyvy,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);








LIBYUV_API
int BGRAToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int ABGRToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int RGBAToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


#define BG24ToARGB RGB24ToARGB


LIBYUV_API
int RGB24ToARGB(const uint8* src_frame, int src_stride_frame,
                uint8* dst_argb, int dst_stride_argb,
                int width, int height);


LIBYUV_API
int RAWToARGB(const uint8* src_frame, int src_stride_frame,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height);


LIBYUV_API
int RGB565ToARGB(const uint8* src_frame, int src_stride_frame,
                 uint8* dst_argb, int dst_stride_argb,
                 int width, int height);


LIBYUV_API
int ARGB1555ToARGB(const uint8* src_frame, int src_stride_frame,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);


LIBYUV_API
int ARGB4444ToARGB(const uint8* src_frame, int src_stride_frame,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);

#ifdef HAVE_JPEG


LIBYUV_API
int MJPGToARGB(const uint8* sample, size_t sample_size,
               uint8* dst_argb, int dst_stride_argb,
               int src_width, int src_height,
               int dst_width, int dst_height);
#endif

























LIBYUV_API
int ConvertToARGB(const uint8* src_frame, size_t src_size,
                  uint8* dst_argb, int dst_stride_argb,
                  int crop_x, int crop_y,
                  int src_width, int src_height,
                  int dst_width, int dst_height,
                  RotationMode rotation,
                  uint32 format);

#ifdef __cplusplus
}  
}  
#endif

#endif  
