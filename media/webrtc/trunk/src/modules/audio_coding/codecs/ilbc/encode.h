

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENCODE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENCODE_H_

#include "defines.h"





void WebRtcIlbcfix_EncodeImpl(
    WebRtc_UWord16 *bytes,     
    const WebRtc_Word16 *block, 
    iLBC_Enc_Inst_t *iLBCenc_inst 

                          );

#endif
