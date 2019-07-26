

















#include "defines.h"
#include "constants.h"





WebRtc_Word16 WebRtcIlbcfix_GainQuant( 
    WebRtc_Word16 gain, 
    WebRtc_Word16 maxIn, 
    WebRtc_Word16 stage, 
    WebRtc_Word16 *index 
                                        ) {

  WebRtc_Word16 scale, returnVal, cblen;
  WebRtc_Word32 gainW32, measure1, measure2;
  const WebRtc_Word16 *cbPtr, *cb;
  int loc, noMoves, noChecks, i;

  

  scale = WEBRTC_SPL_MAX(1638, maxIn);

  


  cb = WebRtcIlbcfix_kGain[stage];
  cblen = 32>>stage;
  noChecks = 4-stage;

  

  gainW32 = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)gain, 14);

  





  loc = cblen>>1;
  noMoves = loc;
  cbPtr = cb + loc; 

  for (i=noChecks;i>0;i--) {
    noMoves>>=1;
    measure1=WEBRTC_SPL_MUL_16_16(scale, (*cbPtr));

    
    measure1 = measure1 - gainW32;

    if (0>measure1) {
      cbPtr+=noMoves;
      loc+=noMoves;
    } else {
      cbPtr-=noMoves;
      loc-=noMoves;
    }
  }

  

  measure1=WEBRTC_SPL_MUL_16_16(scale, (*cbPtr));
  if (gainW32>measure1) {
    
    measure2=WEBRTC_SPL_MUL_16_16(scale, (*(cbPtr+1)));
    if ((measure2-gainW32)<(gainW32-measure1)) {
      loc+=1;
    }
  } else {
    
    measure2=WEBRTC_SPL_MUL_16_16(scale, (*(cbPtr-1)));
    if ((gainW32-measure2)<=(measure1-gainW32)) {
      loc-=1;
    }
  }

  

  loc=WEBRTC_SPL_MIN(loc, (cblen-1));
  *index=loc;

  
  returnVal=(WebRtc_Word16)((WEBRTC_SPL_MUL_16_16(scale, cb[loc])+8192)>>14);

  
  return(returnVal);
}
