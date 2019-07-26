

















#include "defines.h"






void WebRtcIlbcfix_BwExpand(
    WebRtc_Word16 *out, 
    WebRtc_Word16 *in,  

    WebRtc_Word16 *coef, 
    WebRtc_Word16 length 
                            ) {
  int i;

  out[0] = in[0];
  for (i = 1; i < length; i++) {
    


    out[i] = (WebRtc_Word16)((WEBRTC_SPL_MUL_16_16(coef[i], in[i])+16384)>>15);
  }
}
