

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODER_INTERPOLATE_LSF_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DECODER_INTERPOLATE_LSF_H_

#include "defines.h"





void WebRtcIlbcfix_DecoderInterpolateLsp(
    WebRtc_Word16 *syntdenum,  
    WebRtc_Word16 *weightdenum, 

    WebRtc_Word16 *lsfdeq,   
    WebRtc_Word16 length,   
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                                          );

#endif
