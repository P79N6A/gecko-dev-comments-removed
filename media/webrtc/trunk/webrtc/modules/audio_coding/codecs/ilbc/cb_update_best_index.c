

















#include "defines.h"
#include "cb_update_best_index.h"
#include "constants.h"

void WebRtcIlbcfix_CbUpdateBestIndex(
    WebRtc_Word32 CritNew,    
    WebRtc_Word16 CritNewSh,   
    WebRtc_Word16 IndexNew,   
    WebRtc_Word32 cDotNew,    
    WebRtc_Word16 invEnergyNew,  
    WebRtc_Word16 energyShiftNew,  
    WebRtc_Word32 *CritMax,   
    WebRtc_Word16 *shTotMax,   
    WebRtc_Word16 *bestIndex,   

    WebRtc_Word16 *bestGain)   

{
  WebRtc_Word16 shOld, shNew, tmp16;
  WebRtc_Word16 scaleTmp;
  WebRtc_Word32 gainW32;

  
  if (CritNewSh>(*shTotMax)) {
    shOld=WEBRTC_SPL_MIN(31,CritNewSh-(*shTotMax));
    shNew=0;
  } else {
    shOld=0;
    shNew=WEBRTC_SPL_MIN(31,(*shTotMax)-CritNewSh);
  }

  



  if (WEBRTC_SPL_RSHIFT_W32(CritNew, shNew)>
      WEBRTC_SPL_RSHIFT_W32((*CritMax),shOld)) {

    tmp16 = (WebRtc_Word16)WebRtcSpl_NormW32(cDotNew);
    tmp16 = 16 - tmp16;

    




    scaleTmp = -energyShiftNew-tmp16+31;
    scaleTmp = WEBRTC_SPL_MIN(31, scaleTmp);

    gainW32 = WEBRTC_SPL_MUL_16_16_RSFT(
        ((WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(cDotNew, -tmp16)), invEnergyNew, scaleTmp);

    



    if (gainW32>21299) {
      *bestGain=21299;
    } else if (gainW32<-21299) {
      *bestGain=-21299;
    } else {
      *bestGain=(WebRtc_Word16)gainW32;
    }

    *CritMax=CritNew;
    *shTotMax=CritNewSh;
    *bestIndex = IndexNew;
  }

  return;
}
