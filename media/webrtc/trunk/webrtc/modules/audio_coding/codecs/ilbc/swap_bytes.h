

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SWAP_BYTES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SWAP_BYTES_H_

#include "defines.h"





void WebRtcIlbcfix_SwapBytes(
    const uint16_t* input,   
    int16_t wordLength,      
    uint16_t* output         
                              );

#endif
