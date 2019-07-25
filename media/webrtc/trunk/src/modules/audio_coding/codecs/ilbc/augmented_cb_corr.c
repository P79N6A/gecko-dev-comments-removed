

















#include "defines.h"
#include "constants.h"
#include "augmented_cb_corr.h"

void WebRtcIlbcfix_AugmentedCbCorr(
    WebRtc_Word16 *target,   
    WebRtc_Word16 *buffer,   
    WebRtc_Word16 *interpSamples, 

    WebRtc_Word32 *crossDot,  


    WebRtc_Word16 low,    

    WebRtc_Word16 high,   
    WebRtc_Word16 scale)   

{
  int lagcount;
  WebRtc_Word16 ilow;
  WebRtc_Word16 *targetPtr;
  WebRtc_Word32 *crossDotPtr;
  WebRtc_Word16 *iSPtr=interpSamples;

  


  crossDotPtr=crossDot;
  for (lagcount=low; lagcount<=high; lagcount++) {

    ilow = (WebRtc_Word16) (lagcount-4);

    
    (*crossDotPtr) = WebRtcSpl_DotProductWithScale(target, buffer-lagcount, ilow, scale);

    
    (*crossDotPtr) += WebRtcSpl_DotProductWithScale(target+ilow, iSPtr, 4, scale);
    targetPtr = target + lagcount;
    iSPtr += lagcount-ilow;

    
    (*crossDotPtr) += WebRtcSpl_DotProductWithScale(targetPtr, buffer-lagcount, SUBL-lagcount, scale);
    crossDotPtr++;
  }
}
