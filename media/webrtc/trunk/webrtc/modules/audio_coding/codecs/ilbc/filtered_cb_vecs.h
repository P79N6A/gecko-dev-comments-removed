

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_FILTERED_CB_VECS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_FILTERED_CB_VECS_H_

#include "defines.h"







void WebRtcIlbcfix_FilteredCbVecs(
    int16_t *cbvectors, 
    int16_t *CBmem,  

    int lMem,  
    int16_t samples    
                                  );

#endif
