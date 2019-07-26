

















#include "interpolate.h"
#include "lsf_to_poly.h"
#include "defines.h"





void WebRtcIlbcfix_LspInterpolate2PolyDec(
    int16_t *a,   
    int16_t *lsf1,  
    int16_t *lsf2,  
    int16_t coef,  

    int16_t length  
                                          ){
  int16_t lsftmp[LPC_FILTERORDER];

  
  WebRtcIlbcfix_Interpolate(lsftmp, lsf1, lsf2, coef, length);

  
  WebRtcIlbcfix_Lsf2Poly(a, lsftmp);
}
