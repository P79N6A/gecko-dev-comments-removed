

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_INTERFACE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_INTERFACE_H_

#include "defines.h"





int WebRtcIlbcfix_EnhancerInterface( 
    WebRtc_Word16 *out,     
    WebRtc_Word16 *in,      
    iLBC_Dec_Inst_t *iLBCdec_inst 
                                        );

#endif
