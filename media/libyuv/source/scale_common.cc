









#include "libyuv/scale.h"

#include <assert.h>
#include <string.h>

#include "libyuv/cpu_id.h"
#include "libyuv/planar_functions.h"  
#include "libyuv/row.h"
#include "libyuv/scale_row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

static __inline int Abs(int v) {
  return v >= 0 ? v : -v;
}


void ScaleRowDown2_C(const uint8* src_ptr, ptrdiff_t src_stride,
                     uint8* dst, int dst_width) {
  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = src_ptr[1];
    dst[1] = src_ptr[3];
    dst += 2;
    src_ptr += 4;
  }
  if (dst_width & 1) {
    dst[0] = src_ptr[1];
  }
}

void ScaleRowDown2Linear_C(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst, int dst_width) {
  const uint8* s = src_ptr;
  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = (s[0] + s[1] + 1) >> 1;
    dst[1] = (s[2] + s[3] + 1) >> 1;
    dst += 2;
    s += 4;
  }
  if (dst_width & 1) {
    dst[0] = (s[0] + s[1] + 1) >> 1;
  }
}

void ScaleRowDown2Box_C(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst, int dst_width) {
  const uint8* s = src_ptr;
  const uint8* t = src_ptr + src_stride;
  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = (s[0] + s[1] + t[0] + t[1] + 2) >> 2;
    dst[1] = (s[2] + s[3] + t[2] + t[3] + 2) >> 2;
    dst += 2;
    s += 4;
    t += 4;
  }
  if (dst_width & 1) {
    dst[0] = (s[0] + s[1] + t[0] + t[1] + 2) >> 2;
  }
}

void ScaleRowDown4_C(const uint8* src_ptr, ptrdiff_t src_stride,
                     uint8* dst, int dst_width) {
  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = src_ptr[2];
    dst[1] = src_ptr[6];
    dst += 2;
    src_ptr += 8;
  }
  if (dst_width & 1) {
    dst[0] = src_ptr[2];
  }
}

void ScaleRowDown4Box_C(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst, int dst_width) {
  intptr_t stride = src_stride;
  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] + src_ptr[3] +
             src_ptr[stride + 0] + src_ptr[stride + 1] +
             src_ptr[stride + 2] + src_ptr[stride + 3] +
             src_ptr[stride * 2 + 0] + src_ptr[stride * 2 + 1] +
             src_ptr[stride * 2 + 2] + src_ptr[stride * 2 + 3] +
             src_ptr[stride * 3 + 0] + src_ptr[stride * 3 + 1] +
             src_ptr[stride * 3 + 2] + src_ptr[stride * 3 + 3] +
             8) >> 4;
    dst[1] = (src_ptr[4] + src_ptr[5] + src_ptr[6] + src_ptr[7] +
             src_ptr[stride + 4] + src_ptr[stride + 5] +
             src_ptr[stride + 6] + src_ptr[stride + 7] +
             src_ptr[stride * 2 + 4] + src_ptr[stride * 2 + 5] +
             src_ptr[stride * 2 + 6] + src_ptr[stride * 2 + 7] +
             src_ptr[stride * 3 + 4] + src_ptr[stride * 3 + 5] +
             src_ptr[stride * 3 + 6] + src_ptr[stride * 3 + 7] +
             8) >> 4;
    dst += 2;
    src_ptr += 8;
  }
  if (dst_width & 1) {
    dst[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] + src_ptr[3] +
             src_ptr[stride + 0] + src_ptr[stride + 1] +
             src_ptr[stride + 2] + src_ptr[stride + 3] +
             src_ptr[stride * 2 + 0] + src_ptr[stride * 2 + 1] +
             src_ptr[stride * 2 + 2] + src_ptr[stride * 2 + 3] +
             src_ptr[stride * 3 + 0] + src_ptr[stride * 3 + 1] +
             src_ptr[stride * 3 + 2] + src_ptr[stride * 3 + 3] +
             8) >> 4;
  }
}

void ScaleRowDown34_C(const uint8* src_ptr, ptrdiff_t src_stride,
                      uint8* dst, int dst_width) {
  int x;
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (x = 0; x < dst_width; x += 3) {
    dst[0] = src_ptr[0];
    dst[1] = src_ptr[1];
    dst[2] = src_ptr[3];
    dst += 3;
    src_ptr += 4;
  }
}


void ScaleRowDown34_0_Box_C(const uint8* src_ptr, ptrdiff_t src_stride,
                            uint8* d, int dst_width) {
  const uint8* s = src_ptr;
  const uint8* t = src_ptr + src_stride;
  int x;
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (x = 0; x < dst_width; x += 3) {
    uint8 a0 = (s[0] * 3 + s[1] * 1 + 2) >> 2;
    uint8 a1 = (s[1] * 1 + s[2] * 1 + 1) >> 1;
    uint8 a2 = (s[2] * 1 + s[3] * 3 + 2) >> 2;
    uint8 b0 = (t[0] * 3 + t[1] * 1 + 2) >> 2;
    uint8 b1 = (t[1] * 1 + t[2] * 1 + 1) >> 1;
    uint8 b2 = (t[2] * 1 + t[3] * 3 + 2) >> 2;
    d[0] = (a0 * 3 + b0 + 2) >> 2;
    d[1] = (a1 * 3 + b1 + 2) >> 2;
    d[2] = (a2 * 3 + b2 + 2) >> 2;
    d += 3;
    s += 4;
    t += 4;
  }
}


void ScaleRowDown34_1_Box_C(const uint8* src_ptr, ptrdiff_t src_stride,
                            uint8* d, int dst_width) {
  const uint8* s = src_ptr;
  const uint8* t = src_ptr + src_stride;
  int x;
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (x = 0; x < dst_width; x += 3) {
    uint8 a0 = (s[0] * 3 + s[1] * 1 + 2) >> 2;
    uint8 a1 = (s[1] * 1 + s[2] * 1 + 1) >> 1;
    uint8 a2 = (s[2] * 1 + s[3] * 3 + 2) >> 2;
    uint8 b0 = (t[0] * 3 + t[1] * 1 + 2) >> 2;
    uint8 b1 = (t[1] * 1 + t[2] * 1 + 1) >> 1;
    uint8 b2 = (t[2] * 1 + t[3] * 3 + 2) >> 2;
    d[0] = (a0 + b0 + 1) >> 1;
    d[1] = (a1 + b1 + 1) >> 1;
    d[2] = (a2 + b2 + 1) >> 1;
    d += 3;
    s += 4;
    t += 4;
  }
}


void ScaleCols_C(uint8* dst_ptr, const uint8* src_ptr,
                 int dst_width, int x, int dx) {
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    dst_ptr[0] = src_ptr[x >> 16];
    x += dx;
    dst_ptr[1] = src_ptr[x >> 16];
    x += dx;
    dst_ptr += 2;
  }
  if (dst_width & 1) {
    dst_ptr[0] = src_ptr[x >> 16];
  }
}


void ScaleColsUp2_C(uint8* dst_ptr, const uint8* src_ptr,
                    int dst_width, int x, int dx) {
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    dst_ptr[1] = dst_ptr[0] = src_ptr[0];
    src_ptr += 1;
    dst_ptr += 2;
  }
  if (dst_width & 1) {
    dst_ptr[0] = src_ptr[0];
  }
}


#define BLENDER(a, b, f) (uint8)((int)(a) + \
    ((int)(f) * ((int)(b) - (int)(a)) >> 16))

void ScaleFilterCols_C(uint8* dst_ptr, const uint8* src_ptr,
                       int dst_width, int x, int dx) {
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    int xi = x >> 16;
    int a = src_ptr[xi];
    int b = src_ptr[xi + 1];
    dst_ptr[0] = BLENDER(a, b, x & 0xffff);
    x += dx;
    xi = x >> 16;
    a = src_ptr[xi];
    b = src_ptr[xi + 1];
    dst_ptr[1] = BLENDER(a, b, x & 0xffff);
    x += dx;
    dst_ptr += 2;
  }
  if (dst_width & 1) {
    int xi = x >> 16;
    int a = src_ptr[xi];
    int b = src_ptr[xi + 1];
    dst_ptr[0] = BLENDER(a, b, x & 0xffff);
  }
}

void ScaleFilterCols64_C(uint8* dst_ptr, const uint8* src_ptr,
                         int dst_width, int x32, int dx) {
  int64 x = (int64)(x32);
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    int64 xi = x >> 16;
    int a = src_ptr[xi];
    int b = src_ptr[xi + 1];
    dst_ptr[0] = BLENDER(a, b, x & 0xffff);
    x += dx;
    xi = x >> 16;
    a = src_ptr[xi];
    b = src_ptr[xi + 1];
    dst_ptr[1] = BLENDER(a, b, x & 0xffff);
    x += dx;
    dst_ptr += 2;
  }
  if (dst_width & 1) {
    int64 xi = x >> 16;
    int a = src_ptr[xi];
    int b = src_ptr[xi + 1];
    dst_ptr[0] = BLENDER(a, b, x & 0xffff);
  }
}
#undef BLENDER

void ScaleRowDown38_C(const uint8* src_ptr, ptrdiff_t src_stride,
                      uint8* dst, int dst_width) {
  int x;
  assert(dst_width % 3 == 0);
  for (x = 0; x < dst_width; x += 3) {
    dst[0] = src_ptr[0];
    dst[1] = src_ptr[3];
    dst[2] = src_ptr[6];
    dst += 3;
    src_ptr += 8;
  }
}


void ScaleRowDown38_3_Box_C(const uint8* src_ptr,
                            ptrdiff_t src_stride,
                            uint8* dst_ptr, int dst_width) {
  intptr_t stride = src_stride;
  int i;
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (i = 0; i < dst_width; i += 3) {
    dst_ptr[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] +
        src_ptr[stride + 0] + src_ptr[stride + 1] +
        src_ptr[stride + 2] + src_ptr[stride * 2 + 0] +
        src_ptr[stride * 2 + 1] + src_ptr[stride * 2 + 2]) *
        (65536 / 9) >> 16;
    dst_ptr[1] = (src_ptr[3] + src_ptr[4] + src_ptr[5] +
        src_ptr[stride + 3] + src_ptr[stride + 4] +
        src_ptr[stride + 5] + src_ptr[stride * 2 + 3] +
        src_ptr[stride * 2 + 4] + src_ptr[stride * 2 + 5]) *
        (65536 / 9) >> 16;
    dst_ptr[2] = (src_ptr[6] + src_ptr[7] +
        src_ptr[stride + 6] + src_ptr[stride + 7] +
        src_ptr[stride * 2 + 6] + src_ptr[stride * 2 + 7]) *
        (65536 / 6) >> 16;
    src_ptr += 8;
    dst_ptr += 3;
  }
}


void ScaleRowDown38_2_Box_C(const uint8* src_ptr, ptrdiff_t src_stride,
                            uint8* dst_ptr, int dst_width) {
  intptr_t stride = src_stride;
  int i;
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (i = 0; i < dst_width; i += 3) {
    dst_ptr[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] +
        src_ptr[stride + 0] + src_ptr[stride + 1] +
        src_ptr[stride + 2]) * (65536 / 6) >> 16;
    dst_ptr[1] = (src_ptr[3] + src_ptr[4] + src_ptr[5] +
        src_ptr[stride + 3] + src_ptr[stride + 4] +
        src_ptr[stride + 5]) * (65536 / 6) >> 16;
    dst_ptr[2] = (src_ptr[6] + src_ptr[7] +
        src_ptr[stride + 6] + src_ptr[stride + 7]) *
        (65536 / 4) >> 16;
    src_ptr += 8;
    dst_ptr += 3;
  }
}

void ScaleAddRows_C(const uint8* src_ptr, ptrdiff_t src_stride,
                    uint16* dst_ptr, int src_width, int src_height) {
  int x;
  assert(src_width > 0);
  assert(src_height > 0);
  for (x = 0; x < src_width; ++x) {
    const uint8* s = src_ptr + x;
    unsigned int sum = 0u;
    int y;
    for (y = 0; y < src_height; ++y) {
      sum += s[0];
      s += src_stride;
    }
    
    dst_ptr[x] = sum < 65535u ? sum : 65535u;
  }
}

void ScaleARGBRowDown2_C(const uint8* src_argb,
                         ptrdiff_t src_stride,
                         uint8* dst_argb, int dst_width) {
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);

  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = src[1];
    dst[1] = src[3];
    src += 4;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[1];
  }
}

void ScaleARGBRowDown2Linear_C(const uint8* src_argb,
                               ptrdiff_t src_stride,
                               uint8* dst_argb, int dst_width) {
  int x;
  for (x = 0; x < dst_width; ++x) {
    dst_argb[0] = (src_argb[0] + src_argb[4] + 1) >> 1;
    dst_argb[1] = (src_argb[1] + src_argb[5] + 1) >> 1;
    dst_argb[2] = (src_argb[2] + src_argb[6] + 1) >> 1;
    dst_argb[3] = (src_argb[3] + src_argb[7] + 1) >> 1;
    src_argb += 8;
    dst_argb += 4;
  }
}

void ScaleARGBRowDown2Box_C(const uint8* src_argb, ptrdiff_t src_stride,
                            uint8* dst_argb, int dst_width) {
  int x;
  for (x = 0; x < dst_width; ++x) {
    dst_argb[0] = (src_argb[0] + src_argb[4] +
                  src_argb[src_stride] + src_argb[src_stride + 4] + 2) >> 2;
    dst_argb[1] = (src_argb[1] + src_argb[5] +
                  src_argb[src_stride + 1] + src_argb[src_stride + 5] + 2) >> 2;
    dst_argb[2] = (src_argb[2] + src_argb[6] +
                  src_argb[src_stride + 2] + src_argb[src_stride + 6] + 2) >> 2;
    dst_argb[3] = (src_argb[3] + src_argb[7] +
                  src_argb[src_stride + 3] + src_argb[src_stride + 7] + 2) >> 2;
    src_argb += 8;
    dst_argb += 4;
  }
}

void ScaleARGBRowDownEven_C(const uint8* src_argb, ptrdiff_t src_stride,
                            int src_stepx,
                            uint8* dst_argb, int dst_width) {
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);

  int x;
  for (x = 0; x < dst_width - 1; x += 2) {
    dst[0] = src[0];
    dst[1] = src[src_stepx];
    src += src_stepx * 2;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[0];
  }
}

void ScaleARGBRowDownEvenBox_C(const uint8* src_argb,
                               ptrdiff_t src_stride,
                               int src_stepx,
                               uint8* dst_argb, int dst_width) {
  int x;
  for (x = 0; x < dst_width; ++x) {
    dst_argb[0] = (src_argb[0] + src_argb[4] +
                  src_argb[src_stride] + src_argb[src_stride + 4] + 2) >> 2;
    dst_argb[1] = (src_argb[1] + src_argb[5] +
                  src_argb[src_stride + 1] + src_argb[src_stride + 5] + 2) >> 2;
    dst_argb[2] = (src_argb[2] + src_argb[6] +
                  src_argb[src_stride + 2] + src_argb[src_stride + 6] + 2) >> 2;
    dst_argb[3] = (src_argb[3] + src_argb[7] +
                  src_argb[src_stride + 3] + src_argb[src_stride + 7] + 2) >> 2;
    src_argb += src_stepx * 4;
    dst_argb += 4;
  }
}


void ScaleARGBCols_C(uint8* dst_argb, const uint8* src_argb,
                     int dst_width, int x, int dx) {
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    dst[0] = src[x >> 16];
    x += dx;
    dst[1] = src[x >> 16];
    x += dx;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[x >> 16];
  }
}

void ScaleARGBCols64_C(uint8* dst_argb, const uint8* src_argb,
                       int dst_width, int x32, int dx) {
  int64 x = (int64)(x32);
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    dst[0] = src[x >> 16];
    x += dx;
    dst[1] = src[x >> 16];
    x += dx;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[x >> 16];
  }
}


void ScaleARGBColsUp2_C(uint8* dst_argb, const uint8* src_argb,
                        int dst_width, int x, int dx) {
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    dst[1] = dst[0] = src[0];
    src += 1;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[0];
  }
}


#define BLENDER1(a, b, f) ((a) * (0x7f ^ f) + (b) * f) >> 7
#define BLENDERC(a, b, f, s) (uint32)( \
    BLENDER1(((a) >> s) & 255, ((b) >> s) & 255, f) << s)
#define BLENDER(a, b, f) \
    BLENDERC(a, b, f, 24) | BLENDERC(a, b, f, 16) | \
    BLENDERC(a, b, f, 8) | BLENDERC(a, b, f, 0)

void ScaleARGBFilterCols_C(uint8* dst_argb, const uint8* src_argb,
                           int dst_width, int x, int dx) {
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    int xi = x >> 16;
    int xf = (x >> 9) & 0x7f;
    uint32 a = src[xi];
    uint32 b = src[xi + 1];
    dst[0] = BLENDER(a, b, xf);
    x += dx;
    xi = x >> 16;
    xf = (x >> 9) & 0x7f;
    a = src[xi];
    b = src[xi + 1];
    dst[1] = BLENDER(a, b, xf);
    x += dx;
    dst += 2;
  }
  if (dst_width & 1) {
    int xi = x >> 16;
    int xf = (x >> 9) & 0x7f;
    uint32 a = src[xi];
    uint32 b = src[xi + 1];
    dst[0] = BLENDER(a, b, xf);
  }
}

void ScaleARGBFilterCols64_C(uint8* dst_argb, const uint8* src_argb,
                             int dst_width, int x32, int dx) {
  int64 x = (int64)(x32);
  const uint32* src = (const uint32*)(src_argb);
  uint32* dst = (uint32*)(dst_argb);
  int j;
  for (j = 0; j < dst_width - 1; j += 2) {
    int64 xi = x >> 16;
    int xf = (x >> 9) & 0x7f;
    uint32 a = src[xi];
    uint32 b = src[xi + 1];
    dst[0] = BLENDER(a, b, xf);
    x += dx;
    xi = x >> 16;
    xf = (x >> 9) & 0x7f;
    a = src[xi];
    b = src[xi + 1];
    dst[1] = BLENDER(a, b, xf);
    x += dx;
    dst += 2;
  }
  if (dst_width & 1) {
    int64 xi = x >> 16;
    int xf = (x >> 9) & 0x7f;
    uint32 a = src[xi];
    uint32 b = src[xi + 1];
    dst[0] = BLENDER(a, b, xf);
  }
}
#undef BLENDER1
#undef BLENDERC
#undef BLENDER


void ScalePlaneVertical(int src_height,
                        int dst_width, int dst_height,
                        int src_stride, int dst_stride,
                        const uint8* src_argb, uint8* dst_argb,
                        int x, int y, int dy,
                        int bpp, enum FilterMode filtering) {
  
  int dst_width_bytes = dst_width * bpp;
  void (*InterpolateRow)(uint8* dst_argb, const uint8* src_argb,
      ptrdiff_t src_stride, int dst_width, int source_y_fraction) =
      InterpolateRow_C;
  const int max_y = (src_height > 1) ? ((src_height - 1) << 16) - 1 : 0;
  int j;
  assert(bpp >= 1 && bpp <= 4);
  assert(src_height != 0);
  assert(dst_width > 0);
  assert(dst_height > 0);
  src_argb += (x >> 16) * bpp;
#if defined(HAS_INTERPOLATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && dst_width_bytes >= 16) {
    InterpolateRow = InterpolateRow_Any_SSE2;
    if (IS_ALIGNED(dst_width_bytes, 16)) {
      InterpolateRow = InterpolateRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride, 16) &&
          IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
        InterpolateRow = InterpolateRow_SSE2;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && dst_width_bytes >= 16) {
    InterpolateRow = InterpolateRow_Any_SSSE3;
    if (IS_ALIGNED(dst_width_bytes, 16)) {
      InterpolateRow = InterpolateRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride, 16) &&
          IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
        InterpolateRow = InterpolateRow_SSSE3;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_AVX2)
  if (TestCpuFlag(kCpuHasAVX2) && dst_width_bytes >= 32) {
    InterpolateRow = InterpolateRow_Any_AVX2;
    if (IS_ALIGNED(dst_width_bytes, 32)) {
      InterpolateRow = InterpolateRow_AVX2;
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && dst_width_bytes >= 16) {
    InterpolateRow = InterpolateRow_Any_NEON;
    if (IS_ALIGNED(dst_width_bytes, 16)) {
      InterpolateRow = InterpolateRow_NEON;
    }
  }
#endif
#if defined(HAS_INTERPOLATEROWS_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && dst_width_bytes >= 4 &&
      IS_ALIGNED(src_argb, 4) && IS_ALIGNED(src_stride, 4) &&
      IS_ALIGNED(dst_argb, 4) && IS_ALIGNED(dst_stride, 4)) {
    InterpolateRow = InterpolateRow_Any_MIPS_DSPR2;
    if (IS_ALIGNED(dst_width_bytes, 4)) {
      InterpolateRow = InterpolateRow_MIPS_DSPR2;
    }
  }
#endif
  for (j = 0; j < dst_height; ++j) {
    int yi;
    int yf;
    if (y > max_y) {
      y = max_y;
    }
    yi = y >> 16;
    yf = filtering ? ((y >> 8) & 255) : 0;
    InterpolateRow(dst_argb, src_argb + yi * src_stride,
                   src_stride, dst_width_bytes, yf);
    dst_argb += dst_stride;
    y += dy;
  }
}


enum FilterMode ScaleFilterReduce(int src_width, int src_height,
                                  int dst_width, int dst_height,
                                  enum FilterMode filtering) {
  if (src_width < 0) {
    src_width = -src_width;
  }
  if (src_height < 0) {
    src_height = -src_height;
  }
  if (filtering == kFilterBox) {
    
    if (dst_width * 2 >= src_width && dst_height * 2 >= src_height) {
      filtering = kFilterBilinear;
    }
    
    if (dst_width >= src_width || dst_height >= src_height) {
      filtering = kFilterBilinear;
    }
  }
  if (filtering == kFilterBilinear) {
    if (src_height == 1) {
      filtering = kFilterLinear;
    }
    
    if (dst_height == src_height || dst_height * 3 == src_height) {
      filtering = kFilterLinear;
    }
    
    
    if (src_width == 1) {
      filtering = kFilterNone;
    }
  }
  if (filtering == kFilterLinear) {
    if (src_width == 1) {
      filtering = kFilterNone;
    }
    
    if (dst_width == src_width || dst_width * 3 == src_width) {
      filtering = kFilterNone;
    }
  }
  return filtering;
}


int FixedDiv_C(int num, int div) {
  return (int)(((int64)(num) << 16) / div);
}


int FixedDiv1_C(int num, int div) {
  return (int)((((int64)(num) << 16) - 0x00010001) /
                          (div - 1));
}

#define CENTERSTART(dx, s) (dx < 0) ? -((-dx >> 1) + s) : ((dx >> 1) + s)


void ScaleSlope(int src_width, int src_height,
                int dst_width, int dst_height,
                enum FilterMode filtering,
                int* x, int* y, int* dx, int* dy) {
  assert(x != NULL);
  assert(y != NULL);
  assert(dx != NULL);
  assert(dy != NULL);
  assert(src_width != 0);
  assert(src_height != 0);
  assert(dst_width > 0);
  assert(dst_height > 0);
  
  if (dst_width == 1 && src_width >= 32768) {
    dst_width = src_width;
  }
  if (dst_height == 1 && src_height >= 32768) {
    dst_height = src_height;
  }
  if (filtering == kFilterBox) {
    
    *dx = FixedDiv(Abs(src_width), dst_width);
    *dy = FixedDiv(src_height, dst_height);
    *x = 0;
    *y = 0;
  } else if (filtering == kFilterBilinear) {
    
    if (dst_width <= Abs(src_width)) {
      *dx = FixedDiv(Abs(src_width), dst_width);
      *x = CENTERSTART(*dx, -32768);  
    } else if (dst_width > 1) {
      *dx = FixedDiv1(Abs(src_width), dst_width);
      *x = 0;
    }
    if (dst_height <= src_height) {
      *dy = FixedDiv(src_height,  dst_height);
      *y = CENTERSTART(*dy, -32768);  
    } else if (dst_height > 1) {
      *dy = FixedDiv1(src_height, dst_height);
      *y = 0;
    }
  } else if (filtering == kFilterLinear) {
    
    if (dst_width <= Abs(src_width)) {
      *dx = FixedDiv(Abs(src_width), dst_width);
      *x = CENTERSTART(*dx, -32768);  
    } else if (dst_width > 1) {
      *dx = FixedDiv1(Abs(src_width), dst_width);
      *x = 0;
    }
    *dy = FixedDiv(src_height, dst_height);
    *y = *dy >> 1;
  } else {
    
    *dx = FixedDiv(Abs(src_width), dst_width);
    *dy = FixedDiv(src_height, dst_height);
    *x = CENTERSTART(*dx, 0);
    *y = CENTERSTART(*dy, 0);
  }
  
  if (src_width < 0) {
    *x += (dst_width - 1) * *dx;
    *dx = -*dx;
    
  }
}
#undef CENTERSTART

#ifdef __cplusplus
}  
}  
#endif
