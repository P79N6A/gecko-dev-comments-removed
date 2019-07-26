

















#include "defines.h"






void WebRtcIlbcfix_CompCorr(
    WebRtc_Word32 *corr, 
    WebRtc_Word32 *ener, 
    WebRtc_Word16 *buffer, 
    WebRtc_Word16 lag,  
    WebRtc_Word16 bLen, 
    WebRtc_Word16 sRange, 
    WebRtc_Word16 scale 
                            ){
  WebRtc_Word16 *w16ptr;

  w16ptr=&buffer[bLen-sRange-lag];

  
  (*corr)=WebRtcSpl_DotProductWithScale(&buffer[bLen-sRange], w16ptr, sRange, scale);
  (*ener)=WebRtcSpl_DotProductWithScale(w16ptr, w16ptr, sRange, scale);

  

  if (*ener == 0) {
    *corr = 0;
    *ener = 1;
  }
}
