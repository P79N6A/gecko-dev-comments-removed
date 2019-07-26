

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_MY_CORR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_MY_CORR_H_

#include "defines.h"





void WebRtcIlbcfix_MyCorr(
    WebRtc_Word32 *corr,  
    WebRtc_Word16 *seq1,  
    WebRtc_Word16 dim1,  
    const WebRtc_Word16 *seq2, 
    WebRtc_Word16 dim2   
                          );

#endif
