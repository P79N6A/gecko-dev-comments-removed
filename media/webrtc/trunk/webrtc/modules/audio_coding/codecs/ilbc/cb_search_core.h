

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_CORE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_CORE_H_

#include "defines.h"

void WebRtcIlbcfix_CbSearchCore(
    int32_t *cDot,    
    int16_t range,    
    int16_t stage,    
    int16_t *inverseEnergy,  
    int16_t *inverseEnergyShift, 

    int32_t *Crit,    
    int16_t *bestIndex,   


    int32_t *bestCrit,   

    int16_t *bestCritSh);  


#endif
