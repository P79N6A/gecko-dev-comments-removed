

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODE_H_

#include "defines.h"





void WebRtcIlbcfix_DecodeImpl(
    int16_t *decblock,    
    const uint16_t *bytes, 
    iLBC_Dec_Inst_t *iLBCdec_inst, 

    int16_t mode      

                           );

#endif
