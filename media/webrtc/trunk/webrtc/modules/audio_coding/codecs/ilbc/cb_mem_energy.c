

















#include "defines.h"
#include "constants.h"
#include "cb_mem_energy_calc.h"







void WebRtcIlbcfix_CbMemEnergy(
    int16_t range,
    int16_t *CB,   
    int16_t *filteredCB,  
    int16_t lMem,   
    int16_t lTarget,   
    int16_t *energyW16,  
    int16_t *energyShifts, 
    int16_t scale,   
    int16_t base_size  
                               ) {
  int16_t *ppi, *ppo, *pp;
  int32_t energy, tmp32;

  



  
  ppi = CB+lMem-lTarget-1;
  ppo = CB+lMem-1;

  pp=CB+lMem-lTarget;
  energy = WebRtcSpl_DotProductWithScale( pp, pp, lTarget, scale);

  
  energyShifts[0] = (int16_t)WebRtcSpl_NormW32(energy);
  tmp32 = WEBRTC_SPL_LSHIFT_W32(energy, energyShifts[0]);
  energyW16[0] = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32, 16);

  


  WebRtcIlbcfix_CbMemEnergyCalc(energy, range, ppi, ppo, energyW16, energyShifts, scale, 0);

  
  energy=0;
  pp=filteredCB+lMem-lTarget;

  energy = WebRtcSpl_DotProductWithScale( pp, pp, lTarget, scale);

  
  energyShifts[base_size] = (int16_t)WebRtcSpl_NormW32(energy);
  tmp32 = WEBRTC_SPL_LSHIFT_W32(energy, energyShifts[base_size]);
  energyW16[base_size] = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32, 16);

  ppi = filteredCB + lMem - 1 - lTarget;
  ppo = filteredCB + lMem - 1;

  WebRtcIlbcfix_CbMemEnergyCalc(energy, range, ppi, ppo, energyW16, energyShifts, scale, base_size);
}
