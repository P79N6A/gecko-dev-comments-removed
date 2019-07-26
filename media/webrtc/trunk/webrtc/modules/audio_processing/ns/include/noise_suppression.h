









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_INCLUDE_NOISE_SUPPRESSION_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_INCLUDE_NOISE_SUPPRESSION_H_

#include "typedefs.h"

typedef struct NsHandleT NsHandle;

#ifdef __cplusplus
extern "C" {
#endif














int WebRtcNs_Create(NsHandle** NS_inst);












int WebRtcNs_Free(NsHandle* NS_inst);
















int WebRtcNs_Init(NsHandle* NS_inst, WebRtc_UWord32 fs);














int WebRtcNs_set_policy(NsHandle* NS_inst, int mode);




















int WebRtcNs_Process(NsHandle* NS_inst,
                     short* spframe,
                     short* spframe_H,
                     short* outframe,
                     short* outframe_H);











float WebRtcNs_prior_speech_probability(NsHandle* handle);

#ifdef __cplusplus
}
#endif

#endif
