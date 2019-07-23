
















#include "yuv_convert.h"


#include "yuv_row.h"
#include "mozilla/SSE.h"

namespace mozilla {

namespace gfx {


void ConvertYCbCrToRGB32(const uint8* y_buf,
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
  unsigned int y_shift = yuv_type;
  bool has_mmx = supports_mmx();
  bool odd_pic_x = pic_x % 2 != 0;
  int x_width = odd_pic_x ? pic_width - 1 : pic_width;

  for (int y = pic_y; y < pic_height + pic_y; ++y) {
    uint8* rgb_row = rgb_buf + (y - pic_y) * rgb_pitch;
    const uint8* y_ptr = y_buf + y * y_pitch + pic_x;
    const uint8* u_ptr = u_buf + (y >> y_shift) * uv_pitch + (pic_x >> 1);
    const uint8* v_ptr = v_buf + (y >> y_shift) * uv_pitch + (pic_x >> 1);

    if (odd_pic_x) {
      
      
      FastConvertYUVToRGB32Row_C(y_ptr++,
                                 u_ptr++,
                                 v_ptr++,
                                 rgb_row,
                                 1);
      rgb_row += 4;
    }

    if (has_mmx)
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
                                 x_width);
  }

  
  if (has_mmx)
    EMMS();
}

}  
}  
