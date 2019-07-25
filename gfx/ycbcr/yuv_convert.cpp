

















#include "yuv_convert.h"


#include "yuv_row.h"
#include "mozilla/SSE.h"

namespace mozilla {

namespace gfx {
 

const int kFractionBits = 16;
const int kFractionMax = 1 << kFractionBits;
const int kFractionMask = ((1 << kFractionBits) - 1);

NS_GFX_(YUVType) TypeFromSize(int ywidth, 
                              int yheight, 
                              int cbcrwidth, 
                              int cbcrheight)
{
  if (ywidth == cbcrwidth && yheight == cbcrheight) {
    return YV24;
  }
  else if (ywidth / 2 == cbcrwidth && yheight == cbcrheight) {
    return YV16;
  }
  else {
    return YV12;
  }
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


static void FilterRows_C(uint8* ybuf, const uint8* y0_ptr, const uint8* y1_ptr,
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

#ifdef MOZILLA_MAY_SUPPORT_MMX
void FilterRows_MMX(uint8* ybuf, const uint8* y0_ptr, const uint8* y1_ptr,
                    int source_width, int source_y_fraction);
#endif

#ifdef MOZILLA_MAY_SUPPORT_SSE2
void FilterRows_SSE2(uint8* ybuf, const uint8* y0_ptr, const uint8* y1_ptr,
                     int source_width, int source_y_fraction);
#endif

static inline void FilterRows(uint8* ybuf, const uint8* y0_ptr,
                              const uint8* y1_ptr, int source_width,
                              int source_y_fraction) {
#ifdef MOZILLA_MAY_SUPPORT_SSE2
  if (mozilla::supports_sse2()) {
    FilterRows_SSE2(ybuf, y0_ptr, y1_ptr, source_width, source_y_fraction);
    return;
  }
#endif

#ifdef MOZILLA_MAY_SUPPORT_MMX
  if (mozilla::supports_mmx()) {
    FilterRows_MMX(ybuf, y0_ptr, y1_ptr, source_width, source_y_fraction);
    return;
  }
#endif

  FilterRows_C(ybuf, y0_ptr, y1_ptr, source_width, source_y_fraction);
}



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
    } else if (filter & FILTER_BILINEAR_H) {
        LinearScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                                 dest_pixel, width, source_dx);
    } else {

#if defined(MOZILLA_MAY_SUPPORT_SSE) && defined(_MSC_VER) && defined(_M_IX86)
      if(mozilla::supports_sse()) {
        if (width == (source_width * 2)) {
          DoubleYUVToRGB32Row_SSE(y_ptr, u_ptr, v_ptr,
                                  dest_pixel, width);
        } else if ((source_dx & kFractionMask) == 0) {
          
          ConvertYUVToRGB32Row_SSE(y_ptr, u_ptr, v_ptr,
                                   dest_pixel, width,
                                   source_dx >> kFractionBits);
        } else if (source_dx_uv == source_dx) {  
          ScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                             dest_pixel, width, source_dx);
        } else {
          RotateConvertYUVToRGB32Row_SSE(y_ptr, u_ptr, v_ptr,
                                         dest_pixel, width,
                                         source_dx >> kFractionBits,
                                         source_dx_uv >> kFractionBits);
        }
      }
      else {
        ScaleYUVToRGB32Row_C(y_ptr, u_ptr, v_ptr,
                             dest_pixel, width, source_dx);
      }
#else
      ScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                         dest_pixel, width, source_dx);
#endif
    }
  }
  
  if (has_mmx)
    EMMS();
}

}  
}  
