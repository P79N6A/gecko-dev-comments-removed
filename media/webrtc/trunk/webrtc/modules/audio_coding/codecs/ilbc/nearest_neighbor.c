

















#include "defines.h"







void WebRtcIlbcfix_NearestNeighbor(
    int16_t *index, 
    int16_t *array, 
    int16_t value, 
    int16_t arlength 
                                   ){
  int i;
  int16_t diff;
  
  int32_t crit[8];

  
  for(i=0;i<arlength;i++){
    diff=array[i]-value;
    crit[i]=WEBRTC_SPL_MUL_16_16(diff, diff);
  }

  
  *index=WebRtcSpl_MinIndexW32(crit, (int16_t)arlength);
}
