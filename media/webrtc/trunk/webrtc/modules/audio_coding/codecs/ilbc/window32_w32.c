

















#include "defines.h"





void WebRtcIlbcfix_Window32W32(
    int32_t *z,    
    int32_t *x,    
    const int32_t  *y,  
    int16_t N     
                               ) {
  int16_t i;
  int16_t x_low, x_hi, y_low, y_hi;
  int16_t left_shifts;
  int32_t temp;

  left_shifts = (int16_t)WebRtcSpl_NormW32(x[0]);
  WebRtcSpl_VectorBitShiftW32(x, N, x, (int16_t)(-left_shifts));


  


  for (i = 0; i < N; i++) {
    
    x_hi = (int16_t) WEBRTC_SPL_RSHIFT_W32(x[i], 16);
    y_hi = (int16_t) WEBRTC_SPL_RSHIFT_W32(y[i], 16);

    
    temp = WEBRTC_SPL_LSHIFT_W32((int32_t)x_hi, 16);
    x_low = (int16_t) WEBRTC_SPL_RSHIFT_W32((x[i] - temp), 1);

    temp = WEBRTC_SPL_LSHIFT_W32((int32_t)y_hi, 16);
    y_low = (int16_t) WEBRTC_SPL_RSHIFT_W32((y[i] - temp), 1);

    
    temp = WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(x_hi, y_hi), 1);
    temp = (temp + (WEBRTC_SPL_MUL_16_16_RSFT(x_hi, y_low, 14)));

    z[i] = (temp + (WEBRTC_SPL_MUL_16_16_RSFT(x_low, y_hi, 14)));
  }

  WebRtcSpl_VectorBitShiftW32(z, N, z, left_shifts);

  return;
}
