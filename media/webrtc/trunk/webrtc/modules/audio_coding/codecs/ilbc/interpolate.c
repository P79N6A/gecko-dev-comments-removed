

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_Interpolate(
    int16_t *out, 
    int16_t *in1, 
    int16_t *in2, 
    int16_t coef, 
    int16_t length)  
{
  int i;
  int16_t invcoef;

  



  invcoef = 16384 - coef; 
  for (i = 0; i < length; i++) {
    out[i] = (int16_t)((coef * in1[i] + invcoef * in2[i] + 8192) >> 14);
  }

  return;
}
