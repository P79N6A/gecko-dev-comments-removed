

















#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_CbSearchCore(
    WebRtc_Word32 *cDot,    
    WebRtc_Word16 range,    
    WebRtc_Word16 stage,    
    WebRtc_Word16 *inverseEnergy,  
    WebRtc_Word16 *inverseEnergyShift, 

    WebRtc_Word32 *Crit,    
    WebRtc_Word16 *bestIndex,   


    WebRtc_Word32 *bestCrit,   

    WebRtc_Word16 *bestCritSh)   

{
  WebRtc_Word32 maxW32, tmp32;
  WebRtc_Word16 max, sh, tmp16;
  int i;
  WebRtc_Word32 *cDotPtr;
  WebRtc_Word16 cDotSqW16;
  WebRtc_Word16 *inverseEnergyPtr;
  WebRtc_Word32 *critPtr;
  WebRtc_Word16 *inverseEnergyShiftPtr;

  
  if (stage==0) {
    cDotPtr=cDot;
    for (i=0;i<range;i++) {
      *cDotPtr=WEBRTC_SPL_MAX(0, (*cDotPtr));
      cDotPtr++;
    }
  }

  
  maxW32 = WebRtcSpl_MaxAbsValueW32(cDot, range);

  sh = (WebRtc_Word16)WebRtcSpl_NormW32(maxW32);
  cDotPtr = cDot;
  inverseEnergyPtr = inverseEnergy;
  critPtr = Crit;
  inverseEnergyShiftPtr=inverseEnergyShift;
  max=WEBRTC_SPL_WORD16_MIN;

  for (i=0;i<range;i++) {
    
    tmp32 = WEBRTC_SPL_LSHIFT_W32(*cDotPtr,sh);
    tmp16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32,16);
    cDotSqW16 = (WebRtc_Word16)(((WebRtc_Word32)(tmp16)*(tmp16))>>16);

    
    *critPtr=WEBRTC_SPL_MUL_16_16(cDotSqW16, (*inverseEnergyPtr));

    

    if ((*critPtr)!=0) {
      max = WEBRTC_SPL_MAX((*inverseEnergyShiftPtr), max);
    }

    inverseEnergyPtr++;
    inverseEnergyShiftPtr++;
    critPtr++;
    cDotPtr++;
  }

  
  if (max==WEBRTC_SPL_WORD16_MIN) {
    max = 0;
  }

  
  critPtr=Crit;
  inverseEnergyShiftPtr=inverseEnergyShift;
  for (i=0;i<range;i++) {
    

    tmp16 = WEBRTC_SPL_MIN(16, max-(*inverseEnergyShiftPtr));

    (*critPtr)=WEBRTC_SPL_SHIFT_W32((*critPtr),-tmp16);
    critPtr++;
    inverseEnergyShiftPtr++;
  }

  
  *bestIndex = WebRtcSpl_MaxIndexW32(Crit, range);
  *bestCrit = Crit[*bestIndex];

  
  *bestCritSh = 32 - 2*sh + max;

  return;
}
