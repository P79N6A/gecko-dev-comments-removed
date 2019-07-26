

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DO_PLC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DO_PLC_H_

#include "defines.h"






void WebRtcIlbcfix_DoThePlc(
    int16_t *PLCresidual,  
    int16_t *PLClpc,    
    int16_t PLI,     

    int16_t *decresidual,  
    int16_t *lpc,    
    int16_t inlag,    
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                            );

#endif
