

















#include "defines.h"
#include "constants.h"








int16_t WebRtcIlbcfix_Chebyshev(
    
    int16_t x,  
    int16_t *f  
                                      ) {
  int16_t b1_high, b1_low; 
  int32_t b2;
  int32_t tmp1W32;
  int32_t tmp2W32;
  int i;

  b2 = (int32_t)0x1000000; 
  
  tmp1W32 = WEBRTC_SPL_LSHIFT_W32((int32_t)x, 10);
  tmp1W32 += WEBRTC_SPL_LSHIFT_W32((int32_t)f[1], 14);

  for (i = 2; i < 5; i++) {
    tmp2W32 = tmp1W32;

    
    b1_high = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp1W32, 16);
    b1_low = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp1W32-WEBRTC_SPL_LSHIFT_W32(((int32_t)b1_high),16), 1);

    
    tmp1W32 = WEBRTC_SPL_LSHIFT_W32( (WEBRTC_SPL_MUL_16_16(b1_high, x) +
                                      WEBRTC_SPL_MUL_16_16_RSFT(b1_low, x, 15)), 2);

    tmp1W32 -= b2;
    tmp1W32 += WEBRTC_SPL_LSHIFT_W32((int32_t)f[i], 14);

    
    b2 = tmp2W32;
  }

  
  b1_high = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp1W32, 16);
  b1_low = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp1W32-WEBRTC_SPL_LSHIFT_W32(((int32_t)b1_high),16), 1);

  
  tmp1W32 = WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(b1_high, x), 1) +
      WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16_RSFT(b1_low, x, 15), 1);

  tmp1W32 -= b2;
  tmp1W32 += WEBRTC_SPL_LSHIFT_W32((int32_t)f[i], 13);

  
  if (tmp1W32>((int32_t)33553408)) {
    return(WEBRTC_SPL_WORD16_MAX);
  } else if (tmp1W32<((int32_t)-33554432)) {
    return(WEBRTC_SPL_WORD16_MIN);
  } else {
    return((int16_t)WEBRTC_SPL_RSHIFT_W32(tmp1W32, 10));
  }
}
