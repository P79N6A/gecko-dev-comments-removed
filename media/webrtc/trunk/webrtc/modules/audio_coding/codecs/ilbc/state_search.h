

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_STATE_SEARCH_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_STATE_SEARCH_H_

#include "defines.h"





void WebRtcIlbcfix_StateSearch(
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    iLBC_bits *iLBC_encbits,

    WebRtc_Word16 *residual,   
    WebRtc_Word16 *syntDenum,  
    WebRtc_Word16 *weightDenum  
                               );

#endif
