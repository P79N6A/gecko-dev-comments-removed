

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SIMPLE_INTERPOLATE_LSF_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_SIMPLE_INTERPOLATE_LSF_H_

#include "defines.h"





void WebRtcIlbcfix_SimpleInterpolateLsf(
    int16_t *syntdenum, 


    int16_t *weightdenum, 


    int16_t *lsf,  
    int16_t *lsfdeq,  
    int16_t *lsfold,  

    int16_t *lsfdeqold, 

    int16_t length,  
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                                        );

#endif
