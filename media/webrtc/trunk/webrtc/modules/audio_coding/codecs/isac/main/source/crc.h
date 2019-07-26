
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_CRC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_CRC_H_

#include "typedefs.h"

















int16_t WebRtcIsac_GetCrc(
    const int16_t* encoded,
    int16_t        no_of_word8s,
    uint32_t*      crc);



#endif 
