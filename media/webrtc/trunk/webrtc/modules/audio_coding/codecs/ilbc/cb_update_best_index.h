

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_UPDATE_BEST_INDEX_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_UPDATE_BEST_INDEX_H_

#include "defines.h"

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

    int16_t *bestGain);   


#endif
