
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_CRC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_CRC_H_

#include "typedefs.h"

















WebRtc_Word16 WebRtcIsac_GetCrc(
    const WebRtc_Word16* encoded,
    WebRtc_Word16        no_of_word8s,
    WebRtc_UWord32*      crc);



#endif 
