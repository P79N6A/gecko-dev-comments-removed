

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODER_INTERPOLATE_LSF_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODER_INTERPOLATE_LSF_H_

#include "defines.h"





void WebRtcIlbcfix_DecoderInterpolateLsp(
    int16_t *syntdenum,  
    int16_t *weightdenum, 

    int16_t *lsfdeq,   
    int16_t length,   
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                                          );

#endif
