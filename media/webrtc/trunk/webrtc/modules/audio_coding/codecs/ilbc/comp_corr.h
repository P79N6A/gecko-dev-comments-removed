

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_COMP_CORR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_COMP_CORR_H_

#include "defines.h"






void WebRtcIlbcfix_CompCorr(
    int32_t *corr, 
    int32_t *ener, 
    int16_t *buffer, 
    int16_t lag,  
    int16_t bLen, 
    int16_t sRange, 
    int16_t scale 
                            );

#endif
