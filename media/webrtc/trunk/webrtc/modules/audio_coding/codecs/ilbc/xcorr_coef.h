

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_XCORR_COEF_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_XCORR_COEF_H_

#include "defines.h"






int WebRtcIlbcfix_XcorrCoef(
    int16_t *target,  
    int16_t *regressor, 
    int16_t subl,  
    int16_t searchLen, 
    int16_t offset,  
    int16_t step   
                            );

#endif
