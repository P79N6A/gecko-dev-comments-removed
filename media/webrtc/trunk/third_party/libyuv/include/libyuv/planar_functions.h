









#ifndef INCLUDE_LIBYUV_PLANAR_FUNCTIONS_H_  
#define INCLUDE_LIBYUV_PLANAR_FUNCTIONS_H_

#include "libyuv/basic_types.h"


#include "libyuv/convert.h"
#include "libyuv/convert_argb.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


LIBYUV_API
void CopyPlane(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height);


LIBYUV_API
void SetPlane(uint8* dst_y, int dst_stride_y,
              int width, int height,
              uint32 value);


LIBYUV_API
int I400ToI400(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height);



#define I422ToI422 I422Copy
LIBYUV_API
int I422Copy(const uint8* src_y, int src_stride_y,
             const uint8* src_u, int src_stride_u,
             const uint8* src_v, int src_stride_v,
             uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int width, int height);


#define I444ToI444 I444Copy
LIBYUV_API
int I444Copy(const uint8* src_y, int src_stride_y,
             const uint8* src_u, int src_stride_u,
             const uint8* src_v, int src_stride_v,
             uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int width, int height);


LIBYUV_API
int YUY2ToI422(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int UYVYToI422(const uint8* src_uyvy, int src_stride_uyvy,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


LIBYUV_API
int I420ToI400(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               int width, int height);


#define I420ToI420Mirror I420Mirror


LIBYUV_API
int I420Mirror(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);


#define I400ToI400Mirror I400Mirror



LIBYUV_API
int I400Mirror(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height);


#define ARGBToARGBMirror ARGBMirror


LIBYUV_API
int ARGBMirror(const uint8* src_argb, int src_stride_argb,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int NV12ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_uv, int src_stride_uv,
                 uint8* dst_rgb565, int dst_stride_rgb565,
                 int width, int height);


LIBYUV_API
int NV21ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_uv, int src_stride_uv,
                 uint8* dst_rgb565, int dst_stride_rgb565,
                 int width, int height);



LIBYUV_API
int I422ToBGRA(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_bgra, int dst_stride_bgra,
               int width, int height);


LIBYUV_API
int I422ToABGR(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_abgr, int dst_stride_abgr,
               int width, int height);


LIBYUV_API
int I422ToRGBA(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_rgba, int dst_stride_rgba,
               int width, int height);


LIBYUV_API
int I420Rect(uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int x, int y, int width, int height,
             int value_y, int value_u, int value_v);


LIBYUV_API
int ARGBRect(uint8* dst_argb, int dst_stride_argb,
             int x, int y, int width, int height, uint32 value);


LIBYUV_API
int ARGBGrayTo(const uint8* src_argb, int src_stride_argb,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


LIBYUV_API
int ARGBGray(uint8* dst_argb, int dst_stride_argb,
             int x, int y, int width, int height);


LIBYUV_API
int ARGBSepia(uint8* dst_argb, int dst_stride_argb,
              int x, int y, int width, int height);







LIBYUV_API
int ARGBColorMatrix(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_argb, int dst_stride_argb,
                    const int8* matrix_argb,
                    int width, int height);







LIBYUV_API
int RGBColorMatrix(uint8* dst_argb, int dst_stride_argb,
                   const int8* matrix_rgb,
                   int x, int y, int width, int height);



LIBYUV_API
int ARGBColorTable(uint8* dst_argb, int dst_stride_argb,
                   const uint8* table_argb,
                   int x, int y, int width, int height);



LIBYUV_API
int RGBColorTable(uint8* dst_argb, int dst_stride_argb,
                  const uint8* table_argb,
                  int x, int y, int width, int height);




LIBYUV_API
int ARGBLumaColorTable(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_argb, int dst_stride_argb,
                       const uint8* luma_rgb_table,
                       int width, int height);









LIBYUV_API
int ARGBPolynomial(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_argb, int dst_stride_argb,
                   const float* poly,
                   int width, int height);





LIBYUV_API
int ARGBQuantize(uint8* dst_argb, int dst_stride_argb,
                 int scale, int interval_size, int interval_offset,
                 int x, int y, int width, int height);


LIBYUV_API
int ARGBCopy(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int width, int height);


LIBYUV_API
int ARGBCopyAlpha(const uint8* src_argb, int src_stride_argb,
                  uint8* dst_argb, int dst_stride_argb,
                  int width, int height);


LIBYUV_API
int ARGBCopyYToAlpha(const uint8* src_y, int src_stride_y,
                     uint8* dst_argb, int dst_stride_argb,
                     int width, int height);

typedef void (*ARGBBlendRow)(const uint8* src_argb0, const uint8* src_argb1,
                             uint8* dst_argb, int width);


LIBYUV_API
ARGBBlendRow GetARGBBlend();



LIBYUV_API
int ARGBBlend(const uint8* src_argb0, int src_stride_argb0,
              const uint8* src_argb1, int src_stride_argb1,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height);


LIBYUV_API
int ARGBMultiply(const uint8* src_argb0, int src_stride_argb0,
                 const uint8* src_argb1, int src_stride_argb1,
                 uint8* dst_argb, int dst_stride_argb,
                 int width, int height);


LIBYUV_API
int ARGBAdd(const uint8* src_argb0, int src_stride_argb0,
            const uint8* src_argb1, int src_stride_argb1,
            uint8* dst_argb, int dst_stride_argb,
            int width, int height);


LIBYUV_API
int ARGBSubtract(const uint8* src_argb0, int src_stride_argb0,
                 const uint8* src_argb1, int src_stride_argb1,
                 uint8* dst_argb, int dst_stride_argb,
                 int width, int height);


LIBYUV_API
int I422ToYUY2(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height);


LIBYUV_API
int I422ToUYVY(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height);


LIBYUV_API
int ARGBAttenuate(const uint8* src_argb, int src_stride_argb,
                  uint8* dst_argb, int dst_stride_argb,
                  int width, int height);


LIBYUV_API
int ARGBUnattenuate(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height);


LIBYUV_API
int MJPGToARGB(const uint8* sample, size_t sample_size,
               uint8* argb, int argb_stride,
               int w, int h, int dw, int dh);




LIBYUV_API
int ARGBComputeCumulativeSum(const uint8* src_argb, int src_stride_argb,
                             int32* dst_cumsum, int dst_stride32_cumsum,
                             int width, int height);







LIBYUV_API
int ARGBBlur(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int32* dst_cumsum, int dst_stride32_cumsum,
             int width, int height, int radius);


LIBYUV_API
int ARGBShade(const uint8* src_argb, int src_stride_argb,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height, uint32 value);







LIBYUV_API
int ARGBInterpolate(const uint8* src_argb0, int src_stride_argb0,
                    const uint8* src_argb1, int src_stride_argb1,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height, int interpolation);

#if defined(__pnacl__) || defined(__CLR_VER) || defined(COVERAGE_ENABLED) || \
    defined(TARGET_IPHONE_SIMULATOR)
#define LIBYUV_DISABLE_X86
#endif



LIBYUV_API
void ARGBAffineRow_C(const uint8* src_argb, int src_argb_stride,
                     uint8* dst_argb, const float* uv_dudv, int width);

#if !defined(LIBYUV_DISABLE_X86) && \
    (defined(_M_IX86) || defined(__x86_64__) || defined(__i386__))
LIBYUV_API
void ARGBAffineRow_SSE2(const uint8* src_argb, int src_argb_stride,
                        uint8* dst_argb, const float* uv_dudv, int width);
#define HAS_ARGBAFFINEROW_SSE2
#endif  



LIBYUV_API
int ARGBShuffle(const uint8* src_bgra, int src_stride_bgra,
                uint8* dst_argb, int dst_stride_argb,
                const uint8* shuffler, int width, int height);


LIBYUV_API
int ARGBSobelToPlane(const uint8* src_argb, int src_stride_argb,
                     uint8* dst_y, int dst_stride_y,
                     int width, int height);


LIBYUV_API
int ARGBSobel(const uint8* src_argb, int src_stride_argb,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height);


LIBYUV_API
int ARGBSobelXY(const uint8* src_argb, int src_stride_argb,
                uint8* dst_argb, int dst_stride_argb,
                int width, int height);

#ifdef __cplusplus
}  
}  
#endif

#endif  
