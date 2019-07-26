

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_INTERPOLATE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_INTERPOLATE_H_

#include "defines.h"





void WebRtcIlbcfix_Interpolate(
    WebRtc_Word16 *out, 
    WebRtc_Word16 *in1, 
    WebRtc_Word16 *in2, 
    WebRtc_Word16 coef, 
    WebRtc_Word16 length); 

#endif
