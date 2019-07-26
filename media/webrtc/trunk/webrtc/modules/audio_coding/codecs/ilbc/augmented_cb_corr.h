

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_AUGMENTED_CB_CORR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_AUGMENTED_CB_CORR_H_

#include "defines.h"





void WebRtcIlbcfix_AugmentedCbCorr(
    int16_t *target,   
    int16_t *buffer,   
    int16_t *interpSamples, 

    int32_t *crossDot,  


    int16_t low,    

    int16_t high,   
    int16_t scale);   


#endif
