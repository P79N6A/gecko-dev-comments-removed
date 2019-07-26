

















#include "defines.h"
#include "constants.h"
#include "augmented_cb_corr.h"

void WebRtcIlbcfix_AugmentedCbCorr(
    int16_t *target,   
    int16_t *buffer,   
    int16_t *interpSamples, 

    int32_t *crossDot,  


    int16_t low,    

    int16_t high,   
    int16_t scale)   

{
  int lagcount;
  int16_t ilow;
  int16_t *targetPtr;
  int32_t *crossDotPtr;
  int16_t *iSPtr=interpSamples;

  


  crossDotPtr=crossDot;
  for (lagcount=low; lagcount<=high; lagcount++) {

    ilow = (int16_t) (lagcount-4);

    
    (*crossDotPtr) = WebRtcSpl_DotProductWithScale(target, buffer-lagcount, ilow, scale);

    
    (*crossDotPtr) += WebRtcSpl_DotProductWithScale(target+ilow, iSPtr, 4, scale);
    targetPtr = target + lagcount;
    iSPtr += lagcount-ilow;

    
    (*crossDotPtr) += WebRtcSpl_DotProductWithScale(targetPtr, buffer-lagcount, SUBL-lagcount, scale);
    crossDotPtr++;
  }
}
