

















#include "defines.h"






void WebRtcIlbcfix_CompCorr(
    int32_t *corr, 
    int32_t *ener, 
    int16_t *buffer, 
    int16_t lag,  
    int16_t bLen, 
    int16_t sRange, 
    int16_t scale 
                            ){
  int16_t *w16ptr;

  w16ptr=&buffer[bLen-sRange-lag];

  
  (*corr)=WebRtcSpl_DotProductWithScale(&buffer[bLen-sRange], w16ptr, sRange, scale);
  (*ener)=WebRtcSpl_DotProductWithScale(w16ptr, w16ptr, sRange, scale);

  

  if (*ener == 0) {
    *corr = 0;
    *ener = 1;
  }
}
