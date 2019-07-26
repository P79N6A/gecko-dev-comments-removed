

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SIMPLE_INTERPOLATE_LSF_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SIMPLE_INTERPOLATE_LSF_H_

#include "defines.h"





void WebRtcIlbcfix_SimpleInterpolateLsf(
    WebRtc_Word16 *syntdenum, 


    WebRtc_Word16 *weightdenum, 


    WebRtc_Word16 *lsf,  
    WebRtc_Word16 *lsfdeq,  
    WebRtc_Word16 *lsfold,  

    WebRtc_Word16 *lsfdeqold, 

    WebRtc_Word16 length,  
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                                        );

#endif
