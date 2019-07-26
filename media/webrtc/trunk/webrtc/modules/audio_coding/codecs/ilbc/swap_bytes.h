

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SWAP_BYTES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SWAP_BYTES_H_

#include "defines.h"





void WebRtcIlbcfix_SwapBytes(
    const WebRtc_UWord16* input,   
    WebRtc_Word16 wordLength,      
    WebRtc_UWord16* output         
                              );

#endif
