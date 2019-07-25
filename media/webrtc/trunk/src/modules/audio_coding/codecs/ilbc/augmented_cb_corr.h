

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_AUGMENTED_CB_CORR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_AUGMENTED_CB_CORR_H_

#include "defines.h"





void WebRtcIlbcfix_AugmentedCbCorr(
    WebRtc_Word16 *target,   
    WebRtc_Word16 *buffer,   
    WebRtc_Word16 *interpSamples, 

    WebRtc_Word32 *crossDot,  


    WebRtc_Word16 low,    

    WebRtc_Word16 high,   
    WebRtc_Word16 scale);   


#endif
