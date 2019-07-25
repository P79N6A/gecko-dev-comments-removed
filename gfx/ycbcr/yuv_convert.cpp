

















#include "yuv_convert.h"


#include "yuv_row.h"
#define MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE2
#define MOZILLA_SSE_INCLUDE_HEADER_FOR_MMX
#include "mozilla/SSE.h"

#ifdef HAVE_YCBCR_TO_RGB565
void __attribute((noinline)) yv12_to_rgb565_neon(uint16 *dst, const uint8 *y, const uint8 *u, const uint8 *v, int n, int oddflag);
#endif

namespace mozilla {

namespace gfx {
 

const int kFractionBits = 16;
const int kFractionMax = 1 << kFractionBits;
const int kFractionMask = ((1 << kFractionBits) - 1);



NS_GFX_(void) ConvertYCbCrToRGB565(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int pic_x,
                                  int pic_y,
                                  int pic_width,
                                  int pic_height,
                                  int y_pitch,
                                  int uv_pitch,
                                  int rgb_pitch,
                                  YUVType yuv_type)
{
#ifdef HAVE_YCBCR_TO_RGB565
  for (int i = 0; i < pic_height; i++) {
    yv12_to_rgb565_neon((uint16*)rgb_buf + pic_width * i,
                         y_buf + y_pitch * i,
                         u_buf + uv_pitch * (i / 2),
                         v_buf + uv_pitch * (i / 2),
                         pic_width,
                         0);
  }
#endif
}


NS_GFX_(void) ConvertYCbCrToRGB32(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int pic_x,
                                  int pic_y,
                                  int pic_width,
                                  int pic_height,
                                  int y_pitch,
                                  int uv_pitch,
                                  int rgb_pitch,
                                  YUVType yuv_type) {
  unsigned int y_shift = yuv_type == YV12 ? 1 : 0;
  unsigned int x_shift = yuv_type == YV24 ? 0 : 1;
  
  bool has_sse = supports_mmx() && supports_sse();
  
  
  has_sse &= yuv_type != YV24;
  bool odd_pic_x = yuv_type != YV24 && pic_x % 2 != 0;
  int x_width = odd_pic_x ? pic_width - 1 : pic_width;

  for (int y = pic_y; y < pic_height + pic_y; ++y) {
    uint8* rgb_row = rgb_buf + (y - pic_y) * rgb_pitch;
    const uint8* y_ptr = y_buf + y * y_pitch + pic_x;
    const uint8* u_ptr = u_buf + (y >> y_shift) * uv_pitch + (pic_x >> x_shift);
    const uint8* v_ptr = v_buf + (y >> y_shift) * uv_pitch + (pic_x >> x_shift);

    if (odd_pic_x) {
      
      
      FastConvertYUVToRGB32Row_C(y_ptr++,
                                 u_ptr++,
                                 v_ptr++,
                                 rgb_row,
                                 1,
                                 x_shift);
      rgb_row += 4;
    }

    if (has_sse) {
      FastConvertYUVToRGB32Row(y_ptr,
                               u_ptr,
                               v_ptr,
                               rgb_row,
                               x_width);
    }
    else {
      FastConvertYUVToRGB32Row_C(y_ptr,
                                 u_ptr,
                                 v_ptr,
                                 rgb_row,
                                 x_width,
                                 x_shift);
    }
  }

  
  if (has_sse)
    EMMS();
}

#if defined(MOZILLA_COMPILE_WITH_SSE2)


static void FilterRows(uint8* ybuf, const uint8* y0_ptr, const uint8* y1_ptr,
                       int source_width, int source_y_fraction) {
  __m128i zero = _mm_setzero_si128();
  __m128i y1_fraction = _mm_set1_epi16(source_y_fraction);
  __m128i y0_fraction = _mm_set1_epi16(256 - source_y_fraction);

  const __m128i* y0_ptr128 = reinterpret_cast<const __m128i*>(y0_ptr);
  const __m128i* y1_ptr128 = reinterpret_cast<const __m128i*>(y1_ptr);
  __m128i* dest128 = reinterpret_cast<__m128i*>(ybuf);
  __m128i* end128 = reinterpret_cast<__m128i*>(ybuf + source_width);

  do {
    __m128i y0 = _mm_loadu_si128(y0_ptr128);
    __m128i y1 = _mm_loadu_si128(y1_ptr128);
    __m128i y2 = _mm_unpackhi_epi8(y0, zero);
    __m128i y3 = _mm_unpackhi_epi8(y1, zero);
    y0 = _mm_unpacklo_epi8(y0, zero);
    y1 = _mm_unpacklo_epi8(y1, zero);
    y0 = _mm_mullo_epi16(y0, y0_fraction);
    y1 = _mm_mullo_epi16(y1, y1_fraction);
    y2 = _mm_mullo_epi16(y2, y0_fraction);
    y3 = _mm_mullo_epi16(y3, y1_fraction);
    y0 = _mm_add_epi16(y0, y1);
    y2 = _mm_add_epi16(y2, y3);
    y0 = _mm_srli_epi16(y0, 8);
    y2 = _mm_srli_epi16(y2, 8);
    y0 = _mm_packus_epi16(y0, y2);
    *dest128++ = y0;
    ++y0_ptr128;
    ++y1_ptr128;
  } while (dest128 < end128);
}
#elif defined(MOZILLA_COMPILE_WITH_MMX)

static void FilterRows(uint8* ybuf, const uint8* y0_ptr, const uint8* y1_ptr,
                       int source_width, int source_y_fraction) {
  __m64 zero = _mm_setzero_si64();
  __m64 y1_fraction = _mm_set1_pi16(source_y_fraction);
  __m64 y0_fraction = _mm_set1_pi16(256 - source_y_fraction);

  const __m64* y0_ptr64 = reinterpret_cast<const __m64*>(y0_ptr);
  const __m64* y1_ptr64 = reinterpret_cast<const __m64*>(y1_ptr);
  __m64* dest64 = reinterpret_cast<__m64*>(ybuf);
  __m64* end64 = reinterpret_cast<__m64*>(ybuf + source_width);

  do {
    __m64 y0 = *y0_ptr64++;
    __m64 y1 = *y1_ptr64++;
    __m64 y2 = _mm_unpackhi_pi8(y0, zero);
    __m64 y3 = _mm_unpackhi_pi8(y1, zero);
    y0 = _mm_unpacklo_pi8(y0, zero);
    y1 = _mm_unpacklo_pi8(y1, zero);
    y0 = _mm_mullo_pi16(y0, y0_fraction);
    y1 = _mm_mullo_pi16(y1, y1_fraction);
    y2 = _mm_mullo_pi16(y2, y0_fraction);
    y3 = _mm_mullo_pi16(y3, y1_fraction);
    y0 = _mm_add_pi16(y0, y1);
    y2 = _mm_add_pi16(y2, y3);
    y0 = _mm_srli_pi16(y0, 8);
    y2 = _mm_srli_pi16(y2, 8);
    y0 = _mm_packs_pu16(y0, y2);
    *dest64++ = y0;
  } while (dest64 < end64);
}
#else  

static void FilterRows(uint8* ybuf, const uint8* y0_ptr, const uint8* y1_ptr,
                       int source_width, int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  uint8* end = ybuf + source_width;
  do {
    ybuf[0] = (y0_ptr[0] * y0_fraction + y1_ptr[0] * y1_fraction) >> 8;
    ybuf[1] = (y0_ptr[1] * y0_fraction + y1_ptr[1] * y1_fraction) >> 8;
    ybuf[2] = (y0_ptr[2] * y0_fraction + y1_ptr[2] * y1_fraction) >> 8;
    ybuf[3] = (y0_ptr[3] * y0_fraction + y1_ptr[3] * y1_fraction) >> 8;
    ybuf[4] = (y0_ptr[4] * y0_fraction + y1_ptr[4] * y1_fraction) >> 8;
    ybuf[5] = (y0_ptr[5] * y0_fraction + y1_ptr[5] * y1_fraction) >> 8;
    ybuf[6] = (y0_ptr[6] * y0_fraction + y1_ptr[6] * y1_fraction) >> 8;
    ybuf[7] = (y0_ptr[7] * y0_fraction + y1_ptr[7] * y1_fraction) >> 8;
    y0_ptr += 8;
    y1_ptr += 8;
    ybuf += 8;
  } while (ybuf < end);
}
#endif


NS_GFX_(void) ScaleYCbCrToRGB32(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int source_width,
                                int source_height,
                                int width,
                                int height,
                                int y_pitch,
                                int uv_pitch,
                                int rgb_pitch,
                                YUVType yuv_type,
                                Rotate view_rotate,
                                ScaleFilter filter) {
  bool has_mmx = supports_mmx();

  
  
  
  const int kFilterBufferSize = 4096;
  
  
  
  
  if (source_width > kFilterBufferSize || view_rotate)
    filter = FILTER_NONE;

  unsigned int y_shift = yuv_type == YV12 ? 1 : 0;
  
  
  
  
  
  
  
  if ((view_rotate == ROTATE_180) ||
      (view_rotate == ROTATE_270) ||
      (view_rotate == MIRROR_ROTATE_0) ||
      (view_rotate == MIRROR_ROTATE_90)) {
    y_buf += source_width - 1;
    u_buf += source_width / 2 - 1;
    v_buf += source_width / 2 - 1;
    source_width = -source_width;
  }
  
  if ((view_rotate == ROTATE_90) ||
      (view_rotate == ROTATE_180) ||
      (view_rotate == MIRROR_ROTATE_90) ||
      (view_rotate == MIRROR_ROTATE_180)) {
    y_buf += (source_height - 1) * y_pitch;
    u_buf += ((source_height >> y_shift) - 1) * uv_pitch;
    v_buf += ((source_height >> y_shift) - 1) * uv_pitch;
    source_height = -source_height;
  }

  
  if (width == 0 || height == 0)
    return;
  int source_dx = source_width * kFractionMax / width;
  int source_dy = source_height * kFractionMax / height;
  int source_dx_uv = source_dx;

  if ((view_rotate == ROTATE_90) ||
      (view_rotate == ROTATE_270)) {
    int tmp = height;
    height = width;
    width = tmp;
    tmp = source_height;
    source_height = source_width;
    source_width = tmp;
    int original_dx = source_dx;
    int original_dy = source_dy;
    source_dx = ((original_dy >> kFractionBits) * y_pitch) << kFractionBits;
    source_dx_uv = ((original_dy >> kFractionBits) * uv_pitch) << kFractionBits;
    source_dy = original_dx;
    if (view_rotate == ROTATE_90) {
      y_pitch = -1;
      uv_pitch = -1;
      source_height = -source_height;
    } else {
      y_pitch = 1;
      uv_pitch = 1;
    }
  }

  
  
  uint8 yuvbuf[16 + kFilterBufferSize * 3 + 16];
  uint8* ybuf =
      reinterpret_cast<uint8*>(reinterpret_cast<PRUptrdiff>(yuvbuf + 15) & ~15);
  uint8* ubuf = ybuf + kFilterBufferSize;
  uint8* vbuf = ubuf + kFilterBufferSize;
  
  int yscale_fixed = (source_height << kFractionBits) / height;

  
  for (int y = 0; y < height; ++y) {
    uint8* dest_pixel = rgb_buf + y * rgb_pitch;
    int source_y_subpixel = (y * yscale_fixed);
    if (yscale_fixed >= (kFractionMax * 2)) {
      source_y_subpixel += kFractionMax / 2;  
    }
    int source_y = source_y_subpixel >> kFractionBits;

    const uint8* y0_ptr = y_buf + source_y * y_pitch;
    const uint8* y1_ptr = y0_ptr + y_pitch;

    const uint8* u0_ptr = u_buf + (source_y >> y_shift) * uv_pitch;
    const uint8* u1_ptr = u0_ptr + uv_pitch;
    const uint8* v0_ptr = v_buf + (source_y >> y_shift) * uv_pitch;
    const uint8* v1_ptr = v0_ptr + uv_pitch;

    
    int source_y_fraction = (source_y_subpixel & kFractionMask) >> 8;
    int source_uv_fraction =
        ((source_y_subpixel >> y_shift) & kFractionMask) >> 8;

    const uint8* y_ptr = y0_ptr;
    const uint8* u_ptr = u0_ptr;
    const uint8* v_ptr = v0_ptr;
    
    
    if (filter & mozilla::gfx::FILTER_BILINEAR_V) {
      if (yscale_fixed != kFractionMax &&
          source_y_fraction && ((source_y + 1) < source_height)) {
        FilterRows(ybuf, y0_ptr, y1_ptr, source_width, source_y_fraction);
      } else {
        memcpy(ybuf, y0_ptr, source_width);
      }
      y_ptr = ybuf;
      ybuf[source_width] = ybuf[source_width-1];
      int uv_source_width = (source_width + 1) / 2;
      if (yscale_fixed != kFractionMax &&
          source_uv_fraction &&
          (((source_y >> y_shift) + 1) < (source_height >> y_shift))) {
        FilterRows(ubuf, u0_ptr, u1_ptr, uv_source_width, source_uv_fraction);
        FilterRows(vbuf, v0_ptr, v1_ptr, uv_source_width, source_uv_fraction);
      } else {
        memcpy(ubuf, u0_ptr, uv_source_width);
        memcpy(vbuf, v0_ptr, uv_source_width);
      }
      u_ptr = ubuf;
      v_ptr = vbuf;
      ubuf[uv_source_width] = ubuf[uv_source_width - 1];
      vbuf[uv_source_width] = vbuf[uv_source_width - 1];
    }
    if (source_dx == kFractionMax) {  
      FastConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                               dest_pixel, width);
    } else {
      if (filter & FILTER_BILINEAR_H) {
        LinearScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                                 dest_pixel, width, source_dx);
    } else {

#if defined(_MSC_VER) && defined(_M_IX86)
        if (width == (source_width * 2)) {
          DoubleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                              dest_pixel, width);
        } else if ((source_dx & kFractionMask) == 0) {
          
          ConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                               dest_pixel, width,
                               source_dx >> kFractionBits);
        } else if (source_dx_uv == source_dx) {  
          ScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                             dest_pixel, width, source_dx);
        } else {
          RotateConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                                     dest_pixel, width,
                                     source_dx >> kFractionBits,
                                     source_dx_uv >> kFractionBits);
        }
#else
        ScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                           dest_pixel, width, source_dx);
#endif
      }      
    }
  }
  
  if (has_mmx)
    EMMS();
}

}  
}  
