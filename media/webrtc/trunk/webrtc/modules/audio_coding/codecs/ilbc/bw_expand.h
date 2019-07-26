

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_BW_EXPAND_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_BW_EXPAND_H_

#include "defines.h"





void WebRtcIlbcfix_BwExpand(
    int16_t *out, 
    int16_t *in,  

    int16_t *coef, 
    int16_t length 
                            );

#endif
