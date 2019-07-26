

















#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_InterpolateSamples(
    int16_t *interpSamples, 
    int16_t *CBmem,   
    int16_t lMem    
                                      ) {
  int16_t *ppi, *ppo, i, j, temp1, temp2;
  int16_t *tmpPtr;

  

  tmpPtr = interpSamples;
  for (j=0; j<20; j++) {
    temp1 = 0;
    temp2 = 3;
    ppo = CBmem+lMem-4;
    ppi = CBmem+lMem-j-24;
    for (i=0; i<4; i++) {

      *tmpPtr++ = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kAlpha[temp2],*ppo, 15) +
          (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kAlpha[temp1], *ppi, 15);

      ppo++;
      ppi++;
      temp1++;
      temp2--;
    }
  }

  return;
}
