

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_GAIN_QUANT_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_GAIN_QUANT_H_

#include "defines.h"





int16_t WebRtcIlbcfix_GainQuant( 
    int16_t gain, 
    int16_t maxIn, 
    int16_t stage, 
    int16_t *index 
                                       );

#endif
