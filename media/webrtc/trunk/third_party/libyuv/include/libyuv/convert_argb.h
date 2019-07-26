









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


int ARGBCopy(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int width, int height);


int I420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int I422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int I444ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int I411ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int I400ToARGB(const uint8* src_y, int src_stride_y,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int I400ToARGB_Reference(const uint8* src_y, int src_stride_y,
                         uint8* dst_argb, int dst_stride_argb,
                         int width, int height);


int NV12ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int NV21ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_vu, int src_stride_vu,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int M420ToARGB(const uint8* src_m420, int src_stride_m420,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);








int YUY2ToARGB(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int UYVYToARGB(const uint8* src_uyvy, int src_stride_uyvy,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);







int BGRAToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


int ABGRToARGB(const uint8* src_frame, int src_stride_frame,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


#define BG24ToARGB RGB24ToARGB


int RGB24ToARGB(const uint8* src_frame, int src_stride_frame,
                uint8* dst_argb, int dst_stride_argb,
                int width, int height);


int RAWToARGB(const uint8* src_frame, int src_stride_frame,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height);


int RGB565ToARGB(const uint8* src_frame, int src_stride_frame,
                 uint8* dst_argb, int dst_stride_argb,
                 int width, int height);


int ARGB1555ToARGB(const uint8* src_frame, int src_stride_frame,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);


int ARGB4444ToARGB(const uint8* src_frame, int src_stride_frame,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);

#ifdef HAVE_JPEG


int MJPGToARGB(const uint8* sample, size_t sample_size,
               uint8* dst_argb, int dst_stride_argb,
               int src_width, int src_height,
               int dst_width, int dst_height);
#endif

























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
