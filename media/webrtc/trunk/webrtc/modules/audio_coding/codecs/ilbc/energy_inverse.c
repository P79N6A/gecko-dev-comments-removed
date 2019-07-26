



















#include "energy_inverse.h"

void WebRtcIlbcfix_EnergyInverse(
    WebRtc_Word16 *energy,    

    int noOfEnergies)  

{
  WebRtc_Word32 Nom=(WebRtc_Word32)0x1FFFFFFF;
  WebRtc_Word16 *energyPtr;
  int i;

  
  energyPtr=energy;
  for (i=0; i<noOfEnergies; i++) {
    (*energyPtr)=WEBRTC_SPL_MAX((*energyPtr),16384);
    energyPtr++;
  }

  
  energyPtr=energy;
  for (i=0; i<noOfEnergies; i++) {
    (*energyPtr) = (WebRtc_Word16)WebRtcSpl_DivW32W16(Nom, (*energyPtr));
    energyPtr++;
  }
}
