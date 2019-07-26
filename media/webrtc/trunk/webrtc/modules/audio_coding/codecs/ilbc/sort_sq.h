

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SORT_SQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SORT_SQ_H_

#include "defines.h"





void WebRtcIlbcfix_SortSq(
    WebRtc_Word16 *xq,   
    WebRtc_Word16 *index,  
    WebRtc_Word16 x,   
    const WebRtc_Word16 *cb, 
    WebRtc_Word16 cb_size  
                           );

#endif
