









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AECM_INCLUDE_ECHO_CONTROL_MOBILE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AECM_INCLUDE_ECHO_CONTROL_MOBILE_H_

#include <stdlib.h>

#include "webrtc/typedefs.h"

enum {
    AecmFalse = 0,
    AecmTrue
};


#define AECM_UNSPECIFIED_ERROR           12000
#define AECM_UNSUPPORTED_FUNCTION_ERROR  12001
#define AECM_UNINITIALIZED_ERROR         12002
#define AECM_NULL_POINTER_ERROR          12003
#define AECM_BAD_PARAMETER_ERROR         12004


#define AECM_BAD_PARAMETER_WARNING       12100

typedef struct {
    int16_t cngMode;            
    int16_t echoMode;           
} AecmConfig;

#ifdef __cplusplus
extern "C" {
#endif















int32_t WebRtcAecm_Create(void **aecmInst);













int32_t WebRtcAecm_Free(void *aecmInst);














int32_t WebRtcAecm_Init(void* aecmInst, int32_t sampFreq);
















int32_t WebRtcAecm_BufferFarend(void* aecmInst,
                                const int16_t* farend,
                                int16_t nrOfSamples);


























int32_t WebRtcAecm_Process(void* aecmInst,
                           const int16_t* nearendNoisy,
                           const int16_t* nearendClean,
                           int16_t* out,
                           int16_t nrOfSamples,
                           int16_t msInSndCardBuf);















int32_t WebRtcAecm_set_config(void* aecmInst, AecmConfig config);















int32_t WebRtcAecm_get_config(void *aecmInst, AecmConfig *config);















int32_t WebRtcAecm_InitEchoPath(void* aecmInst,
                                const void* echo_path,
                                size_t size_bytes);
















int32_t WebRtcAecm_GetEchoPath(void* aecmInst,
                               void* echo_path,
                               size_t size_bytes);








size_t WebRtcAecm_echo_path_size_bytes();












int32_t WebRtcAecm_get_error_code(void *aecmInst);

#ifdef __cplusplus
}
#endif
#endif
