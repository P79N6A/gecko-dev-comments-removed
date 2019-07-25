

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_BW_EXPAND_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_BW_EXPAND_H_

#include "defines.h"





void WebRtcIlbcfix_BwExpand(
    WebRtc_Word16 *out, 
    WebRtc_Word16 *in,  

    WebRtc_Word16 *coef, 
    WebRtc_Word16 length 
                            );

#endif
