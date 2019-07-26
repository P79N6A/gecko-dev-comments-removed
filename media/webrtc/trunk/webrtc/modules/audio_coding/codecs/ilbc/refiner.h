

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_REFINER_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_REFINER_H_

#include "defines.h"









void WebRtcIlbcfix_Refiner(
    int16_t *updStartPos, 
    int16_t *idata,   
    int16_t idatal,   
    int16_t centerStartPos, 
    int16_t estSegPos,  
    int16_t *surround,  

    int16_t gain    
                           );

#endif
