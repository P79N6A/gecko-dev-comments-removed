









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_MAIN_INTERFACE_PCM16B_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_MAIN_INTERFACE_PCM16B_H_




#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif
















WebRtc_Word16 WebRtcPcm16b_EncodeW16(WebRtc_Word16 *speechIn16b,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *speechOut16b);
















WebRtc_Word16 WebRtcPcm16b_Encode(WebRtc_Word16 *speech16b,
                                  WebRtc_Word16 len,
                                  unsigned char *speech8b);
















WebRtc_Word16 WebRtcPcm16b_DecodeW16(void *inst,
                                     WebRtc_Word16 *speechIn16b,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *speechOut16b,
                                     WebRtc_Word16* speechType);

















WebRtc_Word16 WebRtcPcm16b_Decode(unsigned char *speech8b,
                                  WebRtc_Word16 len,
                                  WebRtc_Word16 *speech16b);

#ifdef __cplusplus
}
#endif

#endif
