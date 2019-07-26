

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_LSF_INTERPOLATE_TO_POLY_ENC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_LSF_INTERPOLATE_TO_POLY_ENC_H_

#include "defines.h"






void WebRtcIlbcfix_LsfInterpolate2PloyEnc(
    int16_t *a,  
    int16_t *lsf1, 
    int16_t *lsf2, 
    int16_t coef, 

    int16_t length 
                                          );

#endif
