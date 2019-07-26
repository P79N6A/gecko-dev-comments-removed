

















#include "defines.h"





void WebRtcIlbcfix_Window32W32(
    WebRtc_Word32 *z,    
    WebRtc_Word32 *x,    
    const WebRtc_Word32  *y,  
    WebRtc_Word16 N     
                               ) {
  WebRtc_Word16 i;
  WebRtc_Word16 x_low, x_hi, y_low, y_hi;
  WebRtc_Word16 left_shifts;
  WebRtc_Word32 temp;

  left_shifts = (WebRtc_Word16)WebRtcSpl_NormW32(x[0]);
  WebRtcSpl_VectorBitShiftW32(x, N, x, (WebRtc_Word16)(-left_shifts));


  


  for (i = 0; i < N; i++) {
    
    x_hi = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(x[i], 16);
    y_hi = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(y[i], 16);

    
    temp = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)x_hi, 16);
    x_low = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32((x[i] - temp), 1);

    temp = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)y_hi, 16);
    y_low = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32((y[i] - temp), 1);

    
    temp = WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(x_hi, y_hi), 1);
    temp = (temp + (WEBRTC_SPL_MUL_16_16_RSFT(x_hi, y_low, 14)));

    z[i] = (temp + (WEBRTC_SPL_MUL_16_16_RSFT(x_low, y_hi, 14)));
  }

  WebRtcSpl_VectorBitShiftW32(z, N, z, left_shifts);

  return;
}
