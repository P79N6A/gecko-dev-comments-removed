














#ifndef WEBRTC_COMMON_AUDIO_VAD_INCLUDE_WEBRTC_VAD_H_
#define WEBRTC_COMMON_AUDIO_VAD_INCLUDE_WEBRTC_VAD_H_

#include <stdlib.h>

#include "typedefs.h"

typedef struct WebRtcVadInst VadInst;

#ifdef __cplusplus
extern "C" {
#endif






size_t WebRtcVad_AssignSize();









int WebRtcVad_Assign(void* memory, VadInst** handle);






int WebRtcVad_Create(VadInst** handle);






int WebRtcVad_Free(VadInst* handle);







int WebRtcVad_Init(VadInst* handle);












int WebRtcVad_set_mode(VadInst* handle, int mode);



















int16_t WebRtcVad_Process(VadInst* vad_inst, int16_t fs, int16_t* speech_frame,
                          int16_t frame_length);

#ifdef __cplusplus
}
#endif

#endif
