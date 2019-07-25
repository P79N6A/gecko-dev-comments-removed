

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_H_

#include "defines.h"






void WebRtcIlbcfix_Enhancer(
    WebRtc_Word16 *odata,   
    WebRtc_Word16 *idata,   
    WebRtc_Word16 idatal,   
    WebRtc_Word16 centerStartPos, 
    WebRtc_Word16 *period,   
    WebRtc_Word16 *plocs,   
    WebRtc_Word16 periodl   
                            );

#endif
