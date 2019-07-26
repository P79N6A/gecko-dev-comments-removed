









#ifndef MODULES_AUDIO_CODING_CODECS_G711_MAIN_INTERFACE_G711_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_G711_MAIN_INTERFACE_G711_INTERFACE_H_

#include "typedefs.h"


#define G711_WEBRTC_SPEECH    1
#define G711_WEBRTC_CNG       2

#ifdef __cplusplus
extern "C" {
#endif




















WebRtc_Word16 WebRtcG711_EncodeA(void *state,
                                 WebRtc_Word16 *speechIn,
                                 WebRtc_Word16 len,
                                 WebRtc_Word16 *encoded);




















WebRtc_Word16 WebRtcG711_EncodeU(void *state,
                                 WebRtc_Word16 *speechIn,
                                 WebRtc_Word16 len,
                                 WebRtc_Word16 *encoded);






















WebRtc_Word16 WebRtcG711_DecodeA(void *state,
                                 WebRtc_Word16 *encoded,
                                 WebRtc_Word16 len,
                                 WebRtc_Word16 *decoded,
                                 WebRtc_Word16 *speechType);






















WebRtc_Word16 WebRtcG711_DecodeU(void *state,
                                 WebRtc_Word16 *encoded,
                                 WebRtc_Word16 len,
                                 WebRtc_Word16 *decoded,
                                 WebRtc_Word16 *speechType);


















int WebRtcG711_DurationEst(void* state,
                           const uint8_t* payload,
                           int payload_length_bytes);
















WebRtc_Word16 WebRtcG711_Version(char* version, WebRtc_Word16 lenBytes);

#ifdef __cplusplus
}
#endif


#endif
