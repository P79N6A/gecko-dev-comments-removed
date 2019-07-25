

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_FILTERED_CB_VECS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_FILTERED_CB_VECS_H_

#include "defines.h"







void WebRtcIlbcfix_FilteredCbVecs(
    WebRtc_Word16 *cbvectors, 
    WebRtc_Word16 *CBmem,  

    int lMem,  
    WebRtc_Word16 samples    
                                  );

#endif
