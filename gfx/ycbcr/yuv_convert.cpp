

















#include "yuv_convert.h"


#include "yuv_row.h"
#include "mozilla/SSE.h"

namespace mozilla {

namespace gfx {


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

    if (has_sse)
      FastConvertYUVToRGB32Row(y_ptr,
                               u_ptr,
                               v_ptr,
                               rgb_row,
                               x_width);
    else
      FastConvertYUVToRGB32Row_C(y_ptr,
                                 u_ptr,
                                 v_ptr,
                                 rgb_row,
                                 x_width,
                                 x_shift);
  }

#ifdef ARCH_CPU_X86_FAMILY
  
  if (has_sse)
    EMMS();
#endif
}


void ScaleYCbCrToRGB32(const uint8* y_buf,
                       const uint8* u_buf,
                       const uint8* v_buf,
                       uint8* rgb_buf,
                       int width,
                       int height,
                       int scaled_width,
                       int scaled_height,
                       int y_pitch,
                       int uv_pitch,
                       int rgb_pitch,
                       YUVType yuv_type,
                       Rotate view_rotate) {
  unsigned int y_shift = yuv_type == YV12 ? 1 : 0;
  unsigned int x_shift = yuv_type == YV24 ? 0 : 1;
  bool has_mmx = supports_mmx();
  
  
  
  
  
  
  
  if ((view_rotate == ROTATE_180) ||
      (view_rotate == ROTATE_270) ||
      (view_rotate == MIRROR_ROTATE_0) ||
      (view_rotate == MIRROR_ROTATE_90)) {
    y_buf += width - 1;
    u_buf += width / 2 - 1;
    v_buf += width / 2 - 1;
    width = -width;
  }
  
  if ((view_rotate == ROTATE_90) ||
      (view_rotate == ROTATE_180) ||
      (view_rotate == MIRROR_ROTATE_90) ||
      (view_rotate == MIRROR_ROTATE_180)) {
    y_buf += (height - 1) * y_pitch;
    u_buf += ((height >> y_shift) - 1) * uv_pitch;
    v_buf += ((height >> y_shift) - 1) * uv_pitch;
    height = -height;
  }

  
  if (scaled_width == 0 || scaled_height == 0)
    return;
  int scaled_dx = width * 16 / scaled_width;
  int scaled_dy = height * 16 / scaled_height;

  int scaled_dx_uv = scaled_dx;

  if ((view_rotate == ROTATE_90) ||
      (view_rotate == ROTATE_270)) {
    int tmp = scaled_height;
    scaled_height = scaled_width;
    scaled_width = tmp;
    tmp = height;
    height = width;
    width = tmp;
    int original_dx = scaled_dx;
    int original_dy = scaled_dy;
    scaled_dx = ((original_dy >> 4) * y_pitch) << 4;
    scaled_dx_uv = ((original_dy >> 4) * uv_pitch) << 4;
    scaled_dy = original_dx;
    if (view_rotate == ROTATE_90) {
      y_pitch = -1;
      uv_pitch = -1;
      height = -height;
    } else {
      y_pitch = 1;
      uv_pitch = 1;
    }
  }

  for (int y = 0; y < scaled_height; ++y) {
    uint8* dest_pixel = rgb_buf + y * rgb_pitch;
    int scaled_y = (y * height / scaled_height);
    const uint8* y_ptr = y_buf + scaled_y * y_pitch;
    const uint8* u_ptr = u_buf + (scaled_y >> y_shift) * uv_pitch;
    const uint8* v_ptr = v_buf + (scaled_y >> y_shift) * uv_pitch;

#if defined(_MSC_VER)
    if (scaled_width == (width * 2)) {
      DoubleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                          dest_pixel, scaled_width);
    } else if ((scaled_dx & 15) == 0) {  
      if (scaled_dx_uv == scaled_dx) {   
        if (scaled_dx == 16) {           
          if (has_mmx)
            FastConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                                     dest_pixel, scaled_width);
          else
            FastConvertYUVToRGB32Row_C(y_ptr, u_ptr, v_ptr,
                                      dest_pixel, scaled_width, x_shift);
        } else {  
          ConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                               dest_pixel, scaled_width, scaled_dx >> 4);
        }
      } else {
        RotateConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                                   dest_pixel, scaled_width,
                                   scaled_dx >> 4, scaled_dx_uv >> 4);
      }
#else
    if (scaled_dx == 16) {           // Not scaled
      if (has_mmx)
        FastConvertYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                                 dest_pixel, scaled_width);
      else
        FastConvertYUVToRGB32Row_C(y_ptr, u_ptr, v_ptr,
                                   dest_pixel, scaled_width, x_shift);
#endif
    } else {
      if (has_mmx) 
        ScaleYUVToRGB32Row(y_ptr, u_ptr, v_ptr,
                           dest_pixel, scaled_width, scaled_dx);
      else
        ScaleYUVToRGB32Row_C(y_ptr, u_ptr, v_ptr,
                             dest_pixel, scaled_width, scaled_dx, x_shift);

    }  
  }

  
  if (has_mmx)
    EMMS();
}

}  
}  
