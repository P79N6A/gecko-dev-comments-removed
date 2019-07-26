

















#include "defines.h"
#include "constants.h"
#include "cb_mem_energy_calc.h"







void WebRtcIlbcfix_CbMemEnergy(
    WebRtc_Word16 range,
    WebRtc_Word16 *CB,   
    WebRtc_Word16 *filteredCB,  
    WebRtc_Word16 lMem,   
    WebRtc_Word16 lTarget,   
    WebRtc_Word16 *energyW16,  
    WebRtc_Word16 *energyShifts, 
    WebRtc_Word16 scale,   
    WebRtc_Word16 base_size  
                               ) {
  WebRtc_Word16 *ppi, *ppo, *pp;
  WebRtc_Word32 energy, tmp32;

  



  
  ppi = CB+lMem-lTarget-1;
  ppo = CB+lMem-1;

  pp=CB+lMem-lTarget;
  energy = WebRtcSpl_DotProductWithScale( pp, pp, lTarget, scale);

  
  energyShifts[0] = (WebRtc_Word16)WebRtcSpl_NormW32(energy);
  tmp32 = WEBRTC_SPL_LSHIFT_W32(energy, energyShifts[0]);
  energyW16[0] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32, 16);

  


  WebRtcIlbcfix_CbMemEnergyCalc(energy, range, ppi, ppo, energyW16, energyShifts, scale, 0);

  
  energy=0;
  pp=filteredCB+lMem-lTarget;

  energy = WebRtcSpl_DotProductWithScale( pp, pp, lTarget, scale);

  
  energyShifts[base_size] = (WebRtc_Word16)WebRtcSpl_NormW32(energy);
  tmp32 = WEBRTC_SPL_LSHIFT_W32(energy, energyShifts[base_size]);
  energyW16[base_size] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32, 16);

  ppi = filteredCB + lMem - 1 - lTarget;
  ppo = filteredCB + lMem - 1;

  WebRtcIlbcfix_CbMemEnergyCalc(energy, range, ppi, ppo, energyW16, energyShifts, scale, base_size);
}
