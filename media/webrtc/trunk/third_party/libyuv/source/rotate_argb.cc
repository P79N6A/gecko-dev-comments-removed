









#include "libyuv/rotate.h"

#include "libyuv/cpu_id.h"
#include "libyuv/convert.h"
#include "libyuv/planar_functions.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif



#if !defined(LIBYUV_DISABLE_X86) && \
    (defined(_M_IX86) || \
    (defined(__x86_64__) && !defined(__native_client__)) || defined(__i386__))
#define HAS_SCALEARGBROWDOWNEVEN_SSE2
void ScaleARGBRowDownEven_SSE2(const uint8* src_ptr, int src_stride,
                               int src_stepx,
                               uint8* dst_ptr, int dst_width);
#endif
#if !defined(LIBYUV_DISABLE_NEON) && !defined(__native_client__) && \
    (defined(__ARM_NEON__) || defined(LIBYUV_NEON))
#define HAS_SCALEARGBROWDOWNEVEN_NEON
void ScaleARGBRowDownEven_NEON(const uint8* src_ptr, int src_stride,
                               int src_stepx,
                               uint8* dst_ptr, int dst_width);
#endif

void ScaleARGBRowDownEven_C(const uint8* src_ptr, int,
                            int src_stepx,
                            uint8* dst_ptr, int dst_width);

static void ARGBTranspose(const uint8* src, int src_stride,
                          uint8* dst, int dst_stride,
                          int width, int height) {
  int i;
  int src_pixel_step = src_stride >> 2;
  void (*ScaleARGBRowDownEven)(const uint8* src_ptr, int src_stride,
      int src_step, uint8* dst_ptr, int dst_width) = ScaleARGBRowDownEven_C;
#if defined(HAS_SCALEARGBROWDOWNEVEN_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(height, 4) &&  
      IS_ALIGNED(dst, 16) && IS_ALIGNED(dst_stride, 16)) {
    ScaleARGBRowDownEven = ScaleARGBRowDownEven_SSE2;
  }
#elif defined(HAS_SCALEARGBROWDOWNEVEN_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(height, 4) &&  
      IS_ALIGNED(src, 4)) {
    ScaleARGBRowDownEven = ScaleARGBRowDownEven_NEON;
  }
#endif

  for (i = 0; i < width; ++i) {  
    ScaleARGBRowDownEven(src, 0, src_pixel_step, dst, height);
    dst += dst_stride;
    src += 4;
  }
}

void ARGBRotate90(const uint8* src, int src_stride,
                  uint8* dst, int dst_stride,
                  int width, int height) {
  
  
  
  src += src_stride * (height - 1);
  src_stride = -src_stride;
  ARGBTranspose(src, src_stride, dst, dst_stride, width, height);
}

void ARGBRotate270(const uint8* src, int src_stride,
                    uint8* dst, int dst_stride,
                    int width, int height) {
  
  
  
  dst += dst_stride * (width - 1);
  dst_stride = -dst_stride;
  ARGBTranspose(src, src_stride, dst, dst_stride, width, height);
}

void ARGBRotate180(const uint8* src, int src_stride,
                   uint8* dst, int dst_stride,
                   int width, int height) {
  
  align_buffer_64(row, width * 4);
  const uint8* src_bot = src + src_stride * (height - 1);
  uint8* dst_bot = dst + dst_stride * (height - 1);
  int half_height = (height + 1) >> 1;
  int y;
  void (*ARGBMirrorRow)(const uint8* src, uint8* dst, int width) =
      ARGBMirrorRow_C;
  void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAS_ARGBMIRRORROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src, 16) && IS_ALIGNED(src_stride, 16) &&
      IS_ALIGNED(dst, 16) && IS_ALIGNED(dst_stride, 16)) {
    ARGBMirrorRow = ARGBMirrorRow_SSSE3;
  }
#endif
#if defined(HAS_ARGBMIRRORROW_AVX2)
  if (TestCpuFlag(kCpuHasAVX2) && IS_ALIGNED(width, 8)) {
    ARGBMirrorRow = ARGBMirrorRow_AVX2;
  }
#endif
#if defined(HAS_ARGBMIRRORROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 4)) {
    ARGBMirrorRow = ARGBMirrorRow_NEON;
  }
#endif
#if defined(HAS_COPYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width * 4, 32)) {
    CopyRow = CopyRow_NEON;
  }
#endif
#if defined(HAS_COPYROW_X86)
  if (TestCpuFlag(kCpuHasX86)) {
    CopyRow = CopyRow_X86;
  }
#endif
#if defined(HAS_COPYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width * 4, 32) &&
      IS_ALIGNED(src, 16) && IS_ALIGNED(src_stride, 16) &&
      IS_ALIGNED(dst, 16) && IS_ALIGNED(dst_stride, 16)) {
    CopyRow = CopyRow_SSE2;
  }
#endif
#if defined(HAS_COPYROW_ERMS)
  if (TestCpuFlag(kCpuHasERMS)) {
    CopyRow = CopyRow_ERMS;
  }
#endif
#if defined(HAS_COPYROW_MIPS)
  if (TestCpuFlag(kCpuHasMIPS)) {
    CopyRow = CopyRow_MIPS;
  }
#endif

  
  for (y = 0; y < half_height; ++y) {
    ARGBMirrorRow(src, row, width);  
    ARGBMirrorRow(src_bot, dst, width);  
    CopyRow(row, dst_bot, width * 4);  
    src += src_stride;
    dst += dst_stride;
    src_bot -= src_stride;
    dst_bot -= dst_stride;
  }
  free_aligned_buffer_64(row);
}

LIBYUV_API
int ARGBRotate(const uint8* src_argb, int src_stride_argb,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height,
               enum RotationMode mode) {
  if (!src_argb || width <= 0 || height == 0 || !dst_argb) {
    return -1;
  }

  
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }

  switch (mode) {
    case kRotate0:
      
      return ARGBCopy(src_argb, src_stride_argb,
                      dst_argb, dst_stride_argb,
                      width, height);
    case kRotate90:
      ARGBRotate90(src_argb, src_stride_argb,
                   dst_argb, dst_stride_argb,
                   width, height);
      return 0;
    case kRotate270:
      ARGBRotate270(src_argb, src_stride_argb,
                    dst_argb, dst_stride_argb,
                    width, height);
      return 0;
    case kRotate180:
      ARGBRotate180(src_argb, src_stride_argb,
                    dst_argb, dst_stride_argb,
                    width, height);
      return 0;
    default:
      break;
  }
  return -1;
}

#ifdef __cplusplus
}  
}  
#endif
