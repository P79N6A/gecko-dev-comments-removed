









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_MAIN_INTERFACE_PCM16B_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_MAIN_INTERFACE_PCM16B_H_




#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif
















int16_t WebRtcPcm16b_EncodeW16(int16_t *speechIn16b,
                               int16_t len,
                               int16_t *speechOut16b);
















int16_t WebRtcPcm16b_Encode(int16_t *speech16b,
                            int16_t len,
                            unsigned char *speech8b);
















int16_t WebRtcPcm16b_DecodeW16(void *inst,
                               int16_t *speechIn16b,
                               int16_t len,
                               int16_t *speechOut16b,
                               int16_t* speechType);

















int16_t WebRtcPcm16b_Decode(unsigned char *speech8b,
                            int16_t len,
                            int16_t *speech16b);

#ifdef __cplusplus
}
#endif

#endif
