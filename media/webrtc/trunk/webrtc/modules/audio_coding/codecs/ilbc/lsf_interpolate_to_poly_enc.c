

















#include "defines.h"
#include "interpolate.h"
#include "lsf_to_poly.h"






void WebRtcIlbcfix_LsfInterpolate2PloyEnc(
    int16_t *a,  
    int16_t *lsf1, 
    int16_t *lsf2, 
    int16_t coef, 

    int16_t length 
                                          ) {
  
  int16_t lsftmp[LPC_FILTERORDER];

  
  WebRtcIlbcfix_Interpolate(lsftmp, lsf1, lsf2, coef, length);

  
  WebRtcIlbcfix_Lsf2Poly(a, lsftmp);

  return;
}
