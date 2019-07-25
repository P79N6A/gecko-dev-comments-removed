



#include "yuv_row.h"

extern "C" {
void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  FastConvertYUVToRGB32Row_C(y_buf, u_buf, v_buf, rgb_buf, width, 1);
}

void ScaleYUVToRGB32Row(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width,
                        int source_dx) {
  ScaleYUVToRGB32Row_C(y_buf, u_buf, v_buf, rgb_buf, width, source_dx);
}

void LinearScaleYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width,
                              int source_dx) {
  LinearScaleYUVToRGB32Row_C(y_buf, u_buf, v_buf, rgb_buf, width, source_dx);
}

} 
