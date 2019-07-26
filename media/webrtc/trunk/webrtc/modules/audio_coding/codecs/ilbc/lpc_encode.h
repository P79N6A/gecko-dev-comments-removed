

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_LPC_ENCODE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_LPC_ENCODE_H_

#include "defines.h"





void WebRtcIlbcfix_LpcEncode(
    int16_t *syntdenum,  

    int16_t *weightdenum, 

    int16_t *lsf_index,  
    int16_t *data,   
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                             );

#endif
