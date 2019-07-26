

















#include "defines.h"






void WebRtcIlbcfix_BwExpand(
    int16_t *out, 
    int16_t *in,  

    int16_t *coef, 
    int16_t length 
                            ) {
  int i;

  out[0] = in[0];
  for (i = 1; i < length; i++) {
    


    out[i] = (int16_t)((WEBRTC_SPL_MUL_16_16(coef[i], in[i])+16384)>>15);
  }
}
