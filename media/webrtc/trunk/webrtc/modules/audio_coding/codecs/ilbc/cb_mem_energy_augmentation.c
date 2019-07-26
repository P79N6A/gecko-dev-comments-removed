

















#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_CbMemEnergyAugmentation(
    int16_t *interpSamples, 
    int16_t *CBmem,   
    int16_t scale,   
    int16_t base_size,  
    int16_t *energyW16,  
    int16_t *energyShifts 
                                           ){
  int32_t energy, tmp32;
  int16_t *ppe, *pp, *interpSamplesPtr;
  int16_t *CBmemPtr, lagcount;
  int16_t *enPtr=&energyW16[base_size-20];
  int16_t *enShPtr=&energyShifts[base_size-20];
  int32_t nrjRecursive;

  CBmemPtr = CBmem+147;
  interpSamplesPtr = interpSamples;

  
  nrjRecursive = WebRtcSpl_DotProductWithScale( CBmemPtr-19, CBmemPtr-19, 15, scale);
  ppe = CBmemPtr - 20;

  for (lagcount=20; lagcount<=39; lagcount++) {

    
    nrjRecursive = nrjRecursive +
        WEBRTC_SPL_MUL_16_16_RSFT(*ppe, *ppe, scale);
    ppe--;
    energy = nrjRecursive;

    
    energy += WebRtcSpl_DotProductWithScale(interpSamplesPtr, interpSamplesPtr, 4, scale);
    interpSamplesPtr += 4;

    
    pp = CBmemPtr - lagcount;
    energy += WebRtcSpl_DotProductWithScale(pp, pp, SUBL-lagcount, scale);

    
    (*enShPtr) = (int16_t)WebRtcSpl_NormW32(energy);
    tmp32 = WEBRTC_SPL_LSHIFT_W32(energy, (*enShPtr));
    (*enPtr) = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32, 16);
    enShPtr++;
    enPtr++;
  }
}
