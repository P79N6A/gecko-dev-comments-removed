

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODE_RESIDUAL_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODE_RESIDUAL_H_

#include "defines.h"





void WebRtcIlbcfix_DecodeResidual(
    iLBC_Dec_Inst_t *iLBCdec_inst,
    
    iLBC_bits *iLBC_encbits, 

    WebRtc_Word16 *decresidual,  
    WebRtc_Word16 *syntdenum   

                                  );

#endif
