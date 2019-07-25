

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_XCORR_COEF_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_XCORR_COEF_H_

#include "defines.h"






int WebRtcIlbcfix_XcorrCoef(
    WebRtc_Word16 *target,  
    WebRtc_Word16 *regressor, 
    WebRtc_Word16 subl,  
    WebRtc_Word16 searchLen, 
    WebRtc_Word16 offset,  
    WebRtc_Word16 step   
                            );

#endif
