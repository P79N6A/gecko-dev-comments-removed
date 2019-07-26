

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DO_PLC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_DO_PLC_H_

#include "defines.h"






void WebRtcIlbcfix_DoThePlc(
    WebRtc_Word16 *PLCresidual,  
    WebRtc_Word16 *PLClpc,    
    WebRtc_Word16 PLI,     

    WebRtc_Word16 *decresidual,  
    WebRtc_Word16 *lpc,    
    WebRtc_Word16 inlag,    
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                            );

#endif
