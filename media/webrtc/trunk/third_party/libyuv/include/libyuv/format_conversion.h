









#ifndef INCLUDE_LIBYUV_FORMATCONVERSION_H_  
#define INCLUDE_LIBYUV_FORMATCONVERSION_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


int BayerBGGRToI420(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_y, int dst_stride_y,
                    uint8* dst_u, int dst_stride_u,
                    uint8* dst_v, int dst_stride_v,
                    int width, int height);

int BayerGBRGToI420(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_y, int dst_stride_y,
                    uint8* dst_u, int dst_stride_u,
                    uint8* dst_v, int dst_stride_v,
                    int width, int height);

int BayerGRBGToI420(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_y, int dst_stride_y,
                    uint8* dst_u, int dst_stride_u,
                    uint8* dst_v, int dst_stride_v,
                    int width, int height);

int BayerRGGBToI420(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_y, int dst_stride_y,
                    uint8* dst_u, int dst_stride_u,
                    uint8* dst_v, int dst_stride_v,
                    int width, int height);


#define BayerRGBToI420(b, bs, f, y, ys, u, us, v, vs, w, h) \
    BayerToI420(b, bs, y, ys, u, us, v, vs, w, h, f)

int BayerToI420(const uint8* src_bayer, int src_stride_bayer,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height,
                uint32 src_fourcc_bayer);


int I420ToBayerBGGR(const uint8* src_y, int src_stride_y,
                    const uint8* src_u, int src_stride_u,
                    const uint8* src_v, int src_stride_v,
                    uint8* dst_frame, int dst_stride_frame,
                    int width, int height);

int I420ToBayerGBRG(const uint8* src_y, int src_stride_y,
                    const uint8* src_u, int src_stride_u,
                    const uint8* src_v, int src_stride_v,
                    uint8* dst_frame, int dst_stride_frame,
                    int width, int height);

int I420ToBayerGRBG(const uint8* src_y, int src_stride_y,
                    const uint8* src_u, int src_stride_u,
                    const uint8* src_v, int src_stride_v,
                    uint8* dst_frame, int dst_stride_frame,
                    int width, int height);

int I420ToBayerRGGB(const uint8* src_y, int src_stride_y,
                    const uint8* src_u, int src_stride_u,
                    const uint8* src_v, int src_stride_v,
                    uint8* dst_frame, int dst_stride_frame,
                    int width, int height);


#define I420ToBayerRGB(y, ys, u, us, v, vs, b, bs, f, w, h) \
    I420ToBayer(y, ys, u, us, v, vs, b, bs, w, h, f)

int I420ToBayer(const uint8* src_y, int src_stride_y,
                const uint8* src_u, int src_stride_u,
                const uint8* src_v, int src_stride_v,
                uint8* dst_frame, int dst_stride_frame,
                int width, int height,
                uint32 dst_fourcc_bayer);


int BayerBGGRToARGB(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height);

int BayerGBRGToARGB(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height);

int BayerGRBGToARGB(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height);

int BayerRGGBToARGB(const uint8* src_bayer, int src_stride_bayer,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height);


#define BayerRGBToARGB(b, bs, f, a, as, w, h) BayerToARGB(b, bs, a, as, w, h, f)

int BayerToARGB(const uint8* src_bayer, int src_stride_bayer,
                uint8* dst_argb, int dst_stride_argb,
                int width, int height,
                uint32 src_fourcc_bayer);


int ARGBToBayerBGGR(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_bayer, int dst_stride_bayer,
                    int width, int height);

int ARGBToBayerGBRG(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_bayer, int dst_stride_bayer,
                    int width, int height);

int ARGBToBayerGRBG(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_bayer, int dst_stride_bayer,
                    int width, int height);

int ARGBToBayerRGGB(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_bayer, int dst_stride_bayer,
                    int width, int height);


#define ARGBToBayerRGB(a, as, b, bs, f, w, h) ARGBToBayer(b, bs, a, as, w, h, f)

int ARGBToBayer(const uint8* src_argb, int src_stride_argb,
                uint8* dst_bayer, int dst_stride_bayer,
                int width, int height,
                uint32 dst_fourcc_bayer);

#ifdef __cplusplus
}  
}  
#endif

#endif  
