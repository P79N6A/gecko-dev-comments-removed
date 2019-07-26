



















#include "energy_inverse.h"

void WebRtcIlbcfix_EnergyInverse(
    int16_t *energy,    

    int noOfEnergies)  

{
  int32_t Nom=(int32_t)0x1FFFFFFF;
  int16_t *energyPtr;
  int i;

  
  energyPtr=energy;
  for (i=0; i<noOfEnergies; i++) {
    (*energyPtr)=WEBRTC_SPL_MAX((*energyPtr),16384);
    energyPtr++;
  }

  
  energyPtr=energy;
  for (i=0; i<noOfEnergies; i++) {
    (*energyPtr) = (int16_t)WebRtcSpl_DivW32W16(Nom, (*energyPtr));
    energyPtr++;
  }
}
