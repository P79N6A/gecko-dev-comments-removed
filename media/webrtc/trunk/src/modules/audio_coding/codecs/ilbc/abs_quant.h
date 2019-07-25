

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ABS_QUANT_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ABS_QUANT_H_

#include "defines.h"






void WebRtcIlbcfix_AbsQuant(
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    iLBC_bits *iLBC_encbits, 


    WebRtc_Word16 *in,     
    WebRtc_Word16 *weightDenum   
                            );

#endif
