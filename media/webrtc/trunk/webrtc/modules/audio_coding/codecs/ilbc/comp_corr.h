

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_COMP_CORR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_COMP_CORR_H_

#include "defines.h"






void WebRtcIlbcfix_CompCorr(
    WebRtc_Word32 *corr, 
    WebRtc_Word32 *ener, 
    WebRtc_Word16 *buffer, 
    WebRtc_Word16 lag,  
    WebRtc_Word16 bLen, 
    WebRtc_Word16 sRange, 
    WebRtc_Word16 scale 
                            );

#endif
