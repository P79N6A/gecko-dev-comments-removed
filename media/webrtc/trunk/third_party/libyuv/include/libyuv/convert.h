









#ifndef INCLUDE_LIBYUV_CONVERT_H_
#define INCLUDE_LIBYUV_CONVERT_H_

#include "libyuv/basic_types.h"
#include "libyuv/rotate.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


int I420ToRGB24(const uint8* src_y, int src_stride_y,
                const uint8* src_u, int src_stride_u,
                const uint8* src_v, int src_stride_v,
                uint8* dst_frame, int dst_stride_frame,
                int width, int height);


int I420ToRAW(const uint8* src_y, int src_stride_y,
              const uint8* src_u, int src_stride_u,
              const uint8* src_v, int src_stride_v,
              uint8* dst_frame, int dst_stride_frame,
              int width, int height);

int I420ToARGB4444(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_frame, int dst_stride_frame,
                   int width, int height);

int I420ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_u, int src_stride_u,
                 const uint8* src_v, int src_stride_v,
                 uint8* dst_frame, int dst_stride_frame,
                 int width, int height);

int I420ToARGB1555(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_frame, int dst_stride_frame,
                   int width, int height);

int I420ToYUY2(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height);

int I422ToYUY2(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height);

int I420ToUYVY(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height);


int RGB24ToARGB(const uint8* src_frame, int src_stride_frame,
                uint8* dst_frame, int dst_stride_frame,
                int width, int height);

int RGB24ToI420(const uint8* src_frame, int src_stride_frame,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height);

int RAWToI420(const uint8* src_frame, int src_stride_frame,
              uint8* dst_y, int dst_stride_y,
              uint8* dst_u, int dst_stride_u,
              uint8* dst_v, int dst_stride_v,
              int width, int height);

int ABGRToI420(const uint8* src_frame, int src_stride_frame,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

int BGRAToI420(const uint8* src_frame, int src_stride_frame,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

int ARGBToI420(const uint8* src_frame, int src_stride_frame,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

int NV12ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_uv, int src_stride_uv,
                 uint8* dst_frame, int dst_stride_frame,
                 int width, int height);























int ConvertToI420(const uint8* src_frame, size_t src_size,
                  uint8* dst_y, int dst_stride_y,
                  uint8* dst_u, int dst_stride_u,
                  uint8* dst_v, int dst_stride_v,
                  int crop_x, int crop_y,
                  int src_width, int src_height,
                  int dst_width, int dst_height,
                  RotationMode rotation,
                  uint32 format);


int ConvertFromI420(const uint8* y, int y_stride,
                    const uint8* u, int u_stride,
                    const uint8* v, int v_stride,
                    uint8* dst_sample, size_t dst_sample_size,
                    int width, int height,
                    uint32 format);

#ifdef __cplusplus
}  
}  
#endif

#endif 
