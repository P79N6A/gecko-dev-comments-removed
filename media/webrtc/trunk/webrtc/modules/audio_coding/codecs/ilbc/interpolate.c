

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_Interpolate(
    WebRtc_Word16 *out, 
    WebRtc_Word16 *in1, 
    WebRtc_Word16 *in2, 
    WebRtc_Word16 coef, 
    WebRtc_Word16 length)  
{
  int i;
  WebRtc_Word16 invcoef;

  



  invcoef = 16384 - coef; 
  for (i = 0; i < length; i++) {
    out[i] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(
        (WEBRTC_SPL_MUL_16_16(coef, in1[i]) + WEBRTC_SPL_MUL_16_16(invcoef, in2[i]))+8192,
        14);
  }

  return;
}
