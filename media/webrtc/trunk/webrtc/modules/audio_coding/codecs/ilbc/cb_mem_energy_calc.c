

















#include "defines.h"




void WebRtcIlbcfix_CbMemEnergyCalc(
    int32_t energy,   
    int16_t range,   
    int16_t *ppi,   
    int16_t *ppo,   
    int16_t *energyW16,  
    int16_t *energyShifts, 
    int16_t scale,   
    int16_t base_size  
                                   )
{
  int16_t j,shft;
  int32_t tmp;
  int16_t *eSh_ptr;
  int16_t *eW16_ptr;


  eSh_ptr  = &energyShifts[1+base_size];
  eW16_ptr = &energyW16[1+base_size];

  for(j=0;j<range-1;j++) {

    

    tmp  = WEBRTC_SPL_MUL_16_16(*ppi, *ppi);
    tmp -= WEBRTC_SPL_MUL_16_16(*ppo, *ppo);
    energy += WEBRTC_SPL_RSHIFT_W32(tmp, scale);
    energy = WEBRTC_SPL_MAX(energy, 0);

    ppi--;
    ppo--;

    


    shft = (int16_t)WebRtcSpl_NormW32(energy);
    *eSh_ptr++ = shft;

    tmp = WEBRTC_SPL_LSHIFT_W32(energy, shft);
    *eW16_ptr++ = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp, 16);
  }
}
