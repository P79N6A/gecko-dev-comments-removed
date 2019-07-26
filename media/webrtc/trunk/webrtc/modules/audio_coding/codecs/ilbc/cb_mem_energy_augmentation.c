

















#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_CbMemEnergyAugmentation(
    WebRtc_Word16 *interpSamples, 
    WebRtc_Word16 *CBmem,   
    WebRtc_Word16 scale,   
    WebRtc_Word16 base_size,  
    WebRtc_Word16 *energyW16,  
    WebRtc_Word16 *energyShifts 
                                           ){
  WebRtc_Word32 energy, tmp32;
  WebRtc_Word16 *ppe, *pp, *interpSamplesPtr;
  WebRtc_Word16 *CBmemPtr, lagcount;
  WebRtc_Word16 *enPtr=&energyW16[base_size-20];
  WebRtc_Word16 *enShPtr=&energyShifts[base_size-20];
  WebRtc_Word32 nrjRecursive;

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

    
    (*enShPtr) = (WebRtc_Word16)WebRtcSpl_NormW32(energy);
    tmp32 = WEBRTC_SPL_LSHIFT_W32(energy, (*enShPtr));
    (*enPtr) = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32, 16);
    enShPtr++;
    enPtr++;
  }
}
