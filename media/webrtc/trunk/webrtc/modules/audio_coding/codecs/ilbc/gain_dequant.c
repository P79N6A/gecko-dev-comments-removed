

















#include "defines.h"
#include "constants.h"






WebRtc_Word16 WebRtcIlbcfix_GainDequant(
    
    WebRtc_Word16 index, 
    WebRtc_Word16 maxIn, 
    WebRtc_Word16 stage 
                                                ){
  WebRtc_Word16 scale;
  const WebRtc_Word16 *gain;

  

  scale=WEBRTC_SPL_ABS_W16(maxIn);
  scale = WEBRTC_SPL_MAX(1638, scale);  

  
  gain = WebRtcIlbcfix_kGain[stage];

  return((WebRtc_Word16)((WEBRTC_SPL_MUL_16_16(scale, gain[index])+8192)>>14));
}
