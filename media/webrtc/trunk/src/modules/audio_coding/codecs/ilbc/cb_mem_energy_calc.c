

















#include "defines.h"




void WebRtcIlbcfix_CbMemEnergyCalc(
    WebRtc_Word32 energy,   
    WebRtc_Word16 range,   
    WebRtc_Word16 *ppi,   
    WebRtc_Word16 *ppo,   
    WebRtc_Word16 *energyW16,  
    WebRtc_Word16 *energyShifts, 
    WebRtc_Word16 scale,   
    WebRtc_Word16 base_size  
                                   )
{
  WebRtc_Word16 j,shft;
  WebRtc_Word32 tmp;
  WebRtc_Word16 *eSh_ptr;
  WebRtc_Word16 *eW16_ptr;


  eSh_ptr  = &energyShifts[1+base_size];
  eW16_ptr = &energyW16[1+base_size];

  for(j=0;j<range-1;j++) {

    

    tmp  = WEBRTC_SPL_MUL_16_16(*ppi, *ppi);
    tmp -= WEBRTC_SPL_MUL_16_16(*ppo, *ppo);
    energy += WEBRTC_SPL_RSHIFT_W32(tmp, scale);
    energy = WEBRTC_SPL_MAX(energy, 0);

    ppi--;
    ppo--;

    


    shft = (WebRtc_Word16)WebRtcSpl_NormW32(energy);
    *eSh_ptr++ = shft;

    tmp = WEBRTC_SPL_LSHIFT_W32(energy, shft);
    *eW16_ptr++ = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp, 16);
  }
}
