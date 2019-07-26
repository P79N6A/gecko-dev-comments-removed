

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODE_H_

#include "defines.h"





void WebRtcIlbcfix_DecodeImpl(
    WebRtc_Word16 *decblock,    
    const WebRtc_UWord16 *bytes, 
    iLBC_Dec_Inst_t *iLBCdec_inst, 

    WebRtc_Word16 mode      

                           );

#endif
