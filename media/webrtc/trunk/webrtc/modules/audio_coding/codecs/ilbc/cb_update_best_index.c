

















#include "defines.h"
#include "cb_update_best_index.h"
#include "constants.h"

void WebRtcIlbcfix_CbUpdateBestIndex(
    int32_t CritNew,    
    int16_t CritNewSh,   
    int16_t IndexNew,   
    int32_t cDotNew,    
    int16_t invEnergyNew,  
    int16_t energyShiftNew,  
    int32_t *CritMax,   
    int16_t *shTotMax,   
    int16_t *bestIndex,   

    int16_t *bestGain)   

{
  int16_t shOld, shNew, tmp16;
  int16_t scaleTmp;
  int32_t gainW32;

  
  if (CritNewSh>(*shTotMax)) {
    shOld=WEBRTC_SPL_MIN(31,CritNewSh-(*shTotMax));
    shNew=0;
  } else {
    shOld=0;
    shNew=WEBRTC_SPL_MIN(31,(*shTotMax)-CritNewSh);
  }

  



  if (WEBRTC_SPL_RSHIFT_W32(CritNew, shNew)>
      WEBRTC_SPL_RSHIFT_W32((*CritMax),shOld)) {

    tmp16 = (int16_t)WebRtcSpl_NormW32(cDotNew);
    tmp16 = 16 - tmp16;

    




    scaleTmp = -energyShiftNew-tmp16+31;
    scaleTmp = WEBRTC_SPL_MIN(31, scaleTmp);

    gainW32 = WEBRTC_SPL_MUL_16_16_RSFT(
        ((int16_t)WEBRTC_SPL_SHIFT_W32(cDotNew, -tmp16)), invEnergyNew, scaleTmp);

    



    if (gainW32>21299) {
      *bestGain=21299;
    } else if (gainW32<-21299) {
      *bestGain=-21299;
    } else {
      *bestGain=(int16_t)gainW32;
    }

    *CritMax=CritNew;
    *shTotMax=CritNewSh;
    *bestIndex = IndexNew;
  }

  return;
}
