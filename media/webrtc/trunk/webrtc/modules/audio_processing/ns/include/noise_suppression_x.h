









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_INCLUDE_NOISE_SUPPRESSION_X_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_INCLUDE_NOISE_SUPPRESSION_X_H_

#include "typedefs.h"

typedef struct NsxHandleT NsxHandle;

#ifdef __cplusplus
extern "C" {
#endif














int WebRtcNsx_Create(NsxHandle** nsxInst);












int WebRtcNsx_Free(NsxHandle* nsxInst);















int WebRtcNsx_Init(NsxHandle* nsxInst, WebRtc_UWord32 fs);














int WebRtcNsx_set_policy(NsxHandle* nsxInst, int mode);



















int WebRtcNsx_Process(NsxHandle* nsxInst,
                      short* speechFrame,
                      short* speechFrameHB,
                      short* outFrame,
                      short* outFrameHB);

#ifdef __cplusplus
}
#endif

#endif
