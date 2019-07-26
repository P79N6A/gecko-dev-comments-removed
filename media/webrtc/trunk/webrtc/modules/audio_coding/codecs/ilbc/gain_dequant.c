

















#include "defines.h"
#include "constants.h"






int16_t WebRtcIlbcfix_GainDequant(
    
    int16_t index, 
    int16_t maxIn, 
    int16_t stage 
                                                ){
  int16_t scale;
  const int16_t *gain;

  

  scale=WEBRTC_SPL_ABS_W16(maxIn);
  scale = WEBRTC_SPL_MAX(1638, scale);  

  
  gain = WebRtcIlbcfix_kGain[stage];

  return((int16_t)((WEBRTC_SPL_MUL_16_16(scale, gain[index])+8192)>>14));
}
