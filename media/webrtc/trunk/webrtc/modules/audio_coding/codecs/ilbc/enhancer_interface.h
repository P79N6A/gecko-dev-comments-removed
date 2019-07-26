

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_INTERFACE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_INTERFACE_H_

#include "defines.h"





int WebRtcIlbcfix_EnhancerInterface( 
    int16_t *out,     
    int16_t *in,      
    iLBC_Dec_Inst_t *iLBCdec_inst 
                                        );

#endif
