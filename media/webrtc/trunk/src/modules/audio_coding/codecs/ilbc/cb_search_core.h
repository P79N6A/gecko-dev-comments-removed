

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_CORE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_CORE_H_

#include "defines.h"

void WebRtcIlbcfix_CbSearchCore(
    WebRtc_Word32 *cDot,    
    WebRtc_Word16 range,    
    WebRtc_Word16 stage,    
    WebRtc_Word16 *inverseEnergy,  
    WebRtc_Word16 *inverseEnergyShift, 

    WebRtc_Word32 *Crit,    
    WebRtc_Word16 *bestIndex,   


    WebRtc_Word32 *bestCrit,   

    WebRtc_Word16 *bestCritSh);  


#endif
