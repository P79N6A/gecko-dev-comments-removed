

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_LPC_ENCODE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_LPC_ENCODE_H_

#include "defines.h"





void WebRtcIlbcfix_LpcEncode(
    WebRtc_Word16 *syntdenum,  

    WebRtc_Word16 *weightdenum, 

    WebRtc_Word16 *lsf_index,  
    WebRtc_Word16 *data,   
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                             );

#endif
