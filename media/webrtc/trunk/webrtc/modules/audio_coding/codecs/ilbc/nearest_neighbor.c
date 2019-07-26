

















#include "defines.h"







void WebRtcIlbcfix_NearestNeighbor(
    WebRtc_Word16 *index, 
    WebRtc_Word16 *array, 
    WebRtc_Word16 value, 
    WebRtc_Word16 arlength 
                                   ){
  int i;
  WebRtc_Word16 diff;
  
  WebRtc_Word32 crit[8];

  
  for(i=0;i<arlength;i++){
    diff=array[i]-value;
    crit[i]=WEBRTC_SPL_MUL_16_16(diff, diff);
  }

  
  *index=WebRtcSpl_MinIndexW32(crit, (WebRtc_Word16)arlength);
}
