

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SPLIT_VQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SPLIT_VQ_H_

#include "defines.h"





void WebRtcIlbcfix_SplitVq(
    int16_t *qX,  
    int16_t *index, 

    int16_t *X,  
    int16_t *CB,  
    int16_t *dim, 
    int16_t *cbsize 
                           );

#endif
