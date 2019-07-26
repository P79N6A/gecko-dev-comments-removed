

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_GAIN_QUANT_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_GAIN_QUANT_H_

#include "defines.h"





WebRtc_Word16 WebRtcIlbcfix_GainQuant( 
    WebRtc_Word16 gain, 
    WebRtc_Word16 maxIn, 
    WebRtc_Word16 stage, 
    WebRtc_Word16 *index 
                                       );

#endif
