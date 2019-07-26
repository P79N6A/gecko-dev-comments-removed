

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENHANCER_H_

#include "defines.h"






void WebRtcIlbcfix_Enhancer(
    int16_t *odata,   
    int16_t *idata,   
    int16_t idatal,   
    int16_t centerStartPos, 
    int16_t *period,   
    int16_t *plocs,   
    int16_t periodl   
                            );

#endif
