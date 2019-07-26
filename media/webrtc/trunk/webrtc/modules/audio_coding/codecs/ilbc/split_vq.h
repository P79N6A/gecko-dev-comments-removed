

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SPLIT_VQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SPLIT_VQ_H_

#include "defines.h"





void WebRtcIlbcfix_SplitVq(
    WebRtc_Word16 *qX,  
    WebRtc_Word16 *index, 

    WebRtc_Word16 *X,  
    WebRtc_Word16 *CB,  
    WebRtc_Word16 *dim, 
    WebRtc_Word16 *cbsize 
                           );

#endif
