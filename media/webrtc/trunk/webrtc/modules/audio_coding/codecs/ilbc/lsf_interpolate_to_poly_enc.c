

















#include "defines.h"
#include "interpolate.h"
#include "lsf_to_poly.h"






void WebRtcIlbcfix_LsfInterpolate2PloyEnc(
    WebRtc_Word16 *a,  
    WebRtc_Word16 *lsf1, 
    WebRtc_Word16 *lsf2, 
    WebRtc_Word16 coef, 

    WebRtc_Word16 length 
                                          ) {
  
  WebRtc_Word16 lsftmp[LPC_FILTERORDER];

  
  WebRtcIlbcfix_Interpolate(lsftmp, lsf1, lsf2, coef, length);

  
  WebRtcIlbcfix_Lsf2Poly(a, lsftmp);

  return;
}
