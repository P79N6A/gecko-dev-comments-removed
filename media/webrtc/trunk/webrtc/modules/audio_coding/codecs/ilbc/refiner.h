

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_REFINER_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_REFINER_H_

#include "defines.h"









void WebRtcIlbcfix_Refiner(
    WebRtc_Word16 *updStartPos, 
    WebRtc_Word16 *idata,   
    WebRtc_Word16 idatal,   
    WebRtc_Word16 centerStartPos, 
    WebRtc_Word16 estSegPos,  
    WebRtc_Word16 *surround,  

    WebRtc_Word16 gain    
                           );

#endif
