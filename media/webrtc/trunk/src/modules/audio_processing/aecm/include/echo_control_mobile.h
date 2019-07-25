









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AECM_INCLUDE_ECHO_CONTROL_MOBILE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AECM_INCLUDE_ECHO_CONTROL_MOBILE_H_

#include "typedefs.h"

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
    WebRtc_Word16 cngMode;            
    WebRtc_Word16 echoMode;           
} AecmConfig;

#ifdef __cplusplus
extern "C" {
#endif















WebRtc_Word32 WebRtcAecm_Create(void **aecmInst);













WebRtc_Word32 WebRtcAecm_Free(void *aecmInst);














WebRtc_Word32 WebRtcAecm_Init(void* aecmInst,
                              WebRtc_Word32 sampFreq);
















WebRtc_Word32 WebRtcAecm_BufferFarend(void* aecmInst,
                                      const WebRtc_Word16* farend,
                                      WebRtc_Word16 nrOfSamples);


























WebRtc_Word32 WebRtcAecm_Process(void* aecmInst,
                                 const WebRtc_Word16* nearendNoisy,
                                 const WebRtc_Word16* nearendClean,
                                 WebRtc_Word16* out,
                                 WebRtc_Word16 nrOfSamples,
                                 WebRtc_Word16 msInSndCardBuf);















WebRtc_Word32 WebRtcAecm_set_config(void* aecmInst,
                                    AecmConfig config);















WebRtc_Word32 WebRtcAecm_get_config(void *aecmInst,
                                    AecmConfig *config);















WebRtc_Word32 WebRtcAecm_InitEchoPath(void* aecmInst,
                                      const void* echo_path,
                                      size_t size_bytes);
















WebRtc_Word32 WebRtcAecm_GetEchoPath(void* aecmInst,
                                     void* echo_path,
                                     size_t size_bytes);








size_t WebRtcAecm_echo_path_size_bytes();












WebRtc_Word32 WebRtcAecm_get_error_code(void *aecmInst);

#ifdef __cplusplus
}
#endif
#endif
