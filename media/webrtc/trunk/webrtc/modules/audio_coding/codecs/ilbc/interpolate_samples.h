

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_INTERPOLATE_SAMPLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_INTERPOLATE_SAMPLES_H_

#include "defines.h"





void WebRtcIlbcfix_InterpolateSamples(
    int16_t *interpSamples, 
    int16_t *CBmem,   
    int16_t lMem    
                                      );

#endif
