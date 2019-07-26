

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SORT_SQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SORT_SQ_H_

#include "defines.h"





void WebRtcIlbcfix_SortSq(
    int16_t *xq,   
    int16_t *index,  
    int16_t x,   
    const int16_t *cb, 
    int16_t cb_size  
                           );

#endif
