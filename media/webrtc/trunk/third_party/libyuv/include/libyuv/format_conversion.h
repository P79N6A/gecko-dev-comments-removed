









#ifndef INCLUDE_LIBYUV_FORMATCONVERSION_H_
#define INCLUDE_LIBYUV_FORMATCONVERSION_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


int BayerRGBToI420(const uint8* src_bayer, int src_stride_bayer,
                   uint32 src_fourcc_bayer,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height);


int BayerRGBToARGB(const uint8* src_bayer, int src_stride_bayer,
                   uint32 src_fourcc_bayer,
                   uint8* dst_rgb, int dst_stride_rgb,
                   int width, int height);


int ARGBToBayerRGB(const uint8* src_rgb, int src_stride_rgb,
                   uint8* dst_bayer, int dst_stride_bayer,
                   uint32 dst_fourcc_bayer,
                   int width, int height);

#ifdef __cplusplus
}  
}  
#endif

#endif  
