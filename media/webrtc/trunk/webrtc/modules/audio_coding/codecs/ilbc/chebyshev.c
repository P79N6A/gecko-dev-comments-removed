

















#include "defines.h"
#include "constants.h"








WebRtc_Word16 WebRtcIlbcfix_Chebyshev(
    
    WebRtc_Word16 x,  
    WebRtc_Word16 *f  
                                      ) {
  WebRtc_Word16 b1_high, b1_low; 
  WebRtc_Word32 b2;
  WebRtc_Word32 tmp1W32;
  WebRtc_Word32 tmp2W32;
  int i;

  b2 = (WebRtc_Word32)0x1000000; 
  
  tmp1W32 = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)x, 10);
  tmp1W32 += WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)f[1], 14);

  for (i = 2; i < 5; i++) {
    tmp2W32 = tmp1W32;

    
    b1_high = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp1W32, 16);
    b1_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp1W32-WEBRTC_SPL_LSHIFT_W32(((WebRtc_Word32)b1_high),16), 1);

    
    tmp1W32 = WEBRTC_SPL_LSHIFT_W32( (WEBRTC_SPL_MUL_16_16(b1_high, x) +
                                      WEBRTC_SPL_MUL_16_16_RSFT(b1_low, x, 15)), 2);

    tmp1W32 -= b2;
    tmp1W32 += WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)f[i], 14);

    
    b2 = tmp2W32;
  }

  
  b1_high = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp1W32, 16);
  b1_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp1W32-WEBRTC_SPL_LSHIFT_W32(((WebRtc_Word32)b1_high),16), 1);

  
  tmp1W32 = WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(b1_high, x), 1) +
      WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16_RSFT(b1_low, x, 15), 1);

  tmp1W32 -= b2;
  tmp1W32 += WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)f[i], 13);

  
  if (tmp1W32>((WebRtc_Word32)33553408)) {
    return(WEBRTC_SPL_WORD16_MAX);
  } else if (tmp1W32<((WebRtc_Word32)-33554432)) {
    return(WEBRTC_SPL_WORD16_MIN);
  } else {
    return((WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp1W32, 10));
  }
}
