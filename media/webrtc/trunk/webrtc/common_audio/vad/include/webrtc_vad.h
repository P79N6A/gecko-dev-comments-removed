














#ifndef WEBRTC_COMMON_AUDIO_VAD_INCLUDE_WEBRTC_VAD_H_  
#define WEBRTC_COMMON_AUDIO_VAD_INCLUDE_WEBRTC_VAD_H_

#include "typedefs.h"  

typedef struct WebRtcVadInst VadInst;

#ifdef __cplusplus
extern "C" {
#endif






int WebRtcVad_Create(VadInst** handle);






int WebRtcVad_Free(VadInst* handle);







int WebRtcVad_Init(VadInst* handle);












int WebRtcVad_set_mode(VadInst* handle, int mode);













int WebRtcVad_Process(VadInst* handle, int fs, int16_t* audio_frame,
                      int frame_length);








int WebRtcVad_ValidRateAndFrameLength(int rate, int frame_length);

#ifdef __cplusplus
}
#endif

#endif
