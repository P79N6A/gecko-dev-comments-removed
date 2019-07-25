

















#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_InterpolateSamples(
    WebRtc_Word16 *interpSamples, 
    WebRtc_Word16 *CBmem,   
    WebRtc_Word16 lMem    
                                      ) {
  WebRtc_Word16 *ppi, *ppo, i, j, temp1, temp2;
  WebRtc_Word16 *tmpPtr;

  

  tmpPtr = interpSamples;
  for (j=0; j<20; j++) {
    temp1 = 0;
    temp2 = 3;
    ppo = CBmem+lMem-4;
    ppi = CBmem+lMem-j-24;
    for (i=0; i<4; i++) {

      *tmpPtr++ = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kAlpha[temp2],*ppo, 15) +
          (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kAlpha[temp1], *ppi, 15);

      ppo++;
      ppi++;
      temp1++;
      temp2--;
    }
  }

  return;
}
