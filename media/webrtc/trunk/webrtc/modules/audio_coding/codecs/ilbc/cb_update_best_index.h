

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_UPDATE_BEST_INDEX_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_UPDATE_BEST_INDEX_H_

#include "defines.h"

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

    WebRtc_Word16 *bestGain);   


#endif
