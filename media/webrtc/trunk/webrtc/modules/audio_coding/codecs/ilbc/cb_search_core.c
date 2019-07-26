

















#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_CbSearchCore(
    int32_t *cDot,    
    int16_t range,    
    int16_t stage,    
    int16_t *inverseEnergy,  
    int16_t *inverseEnergyShift, 

    int32_t *Crit,    
    int16_t *bestIndex,   


    int32_t *bestCrit,   

    int16_t *bestCritSh)   

{
  int32_t maxW32, tmp32;
  int16_t max, sh, tmp16;
  int i;
  int32_t *cDotPtr;
  int16_t cDotSqW16;
  int16_t *inverseEnergyPtr;
  int32_t *critPtr;
  int16_t *inverseEnergyShiftPtr;

  
  if (stage==0) {
    cDotPtr=cDot;
    for (i=0;i<range;i++) {
      *cDotPtr=WEBRTC_SPL_MAX(0, (*cDotPtr));
      cDotPtr++;
    }
  }

  
  maxW32 = WebRtcSpl_MaxAbsValueW32(cDot, range);

  sh = (int16_t)WebRtcSpl_NormW32(maxW32);
  cDotPtr = cDot;
  inverseEnergyPtr = inverseEnergy;
  critPtr = Crit;
  inverseEnergyShiftPtr=inverseEnergyShift;
  max=WEBRTC_SPL_WORD16_MIN;

  for (i=0;i<range;i++) {
    
    tmp32 = WEBRTC_SPL_LSHIFT_W32(*cDotPtr,sh);
    tmp16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32,16);
    cDotSqW16 = (int16_t)(((int32_t)(tmp16)*(tmp16))>>16);

    
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
