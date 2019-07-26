









#ifndef MODULES_AUDIO_CODING_CODECS_G711_MAIN_INTERFACE_G711_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_G711_MAIN_INTERFACE_G711_INTERFACE_H_

#include "typedefs.h"


#define G711_WEBRTC_SPEECH 1
#define G711_WEBRTC_CNG 2

#ifdef __cplusplus
extern "C" {
#endif




















int16_t WebRtcG711_EncodeA(void* state,
                           int16_t* speechIn,
                           int16_t len,
                           int16_t* encoded);




















int16_t WebRtcG711_EncodeU(void* state,
                           int16_t* speechIn,
                           int16_t len,
                           int16_t* encoded);






















int16_t WebRtcG711_DecodeA(void* state,
                           int16_t* encoded,
                           int16_t len,
                           int16_t* decoded,
                           int16_t* speechType);






















int16_t WebRtcG711_DecodeU(void* state,
                           int16_t* encoded,
                           int16_t len,
                           int16_t* decoded,
                           int16_t* speechType);

















int WebRtcG711_DurationEst(void* state,
                           const uint8_t* payload,
                           int payload_length_bytes);
















int16_t WebRtcG711_Version(char* version, int16_t lenBytes);

#ifdef __cplusplus
}
#endif

#endif
