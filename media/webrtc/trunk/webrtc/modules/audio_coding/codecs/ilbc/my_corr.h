

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_MY_CORR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_MY_CORR_H_

#include "defines.h"





void WebRtcIlbcfix_MyCorr(
    int32_t *corr,  
    int16_t *seq1,  
    int16_t dim1,  
    const int16_t *seq2, 
    int16_t dim2   
                          );

#endif
