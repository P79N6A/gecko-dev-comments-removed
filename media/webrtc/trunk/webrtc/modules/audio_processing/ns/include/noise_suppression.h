









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_INCLUDE_NOISE_SUPPRESSION_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_INCLUDE_NOISE_SUPPRESSION_H_

#include "webrtc/typedefs.h"

typedef struct NsHandleT NsHandle;

#ifdef __cplusplus
extern "C" {
#endif














int WebRtcNs_Create(NsHandle** NS_inst);












int WebRtcNs_Free(NsHandle* NS_inst);
















int WebRtcNs_Init(NsHandle* NS_inst, uint32_t fs);














int WebRtcNs_set_policy(NsHandle* NS_inst, int mode);















int WebRtcNs_Analyze(NsHandle* NS_inst, float* spframe);



















int WebRtcNs_Process(NsHandle* NS_inst,
                     float* spframe,
                     float* spframe_H,
                     float* outframe,
                     float* outframe_H);











float WebRtcNs_prior_speech_probability(NsHandle* handle);

#ifdef __cplusplus
}
#endif

#endif
