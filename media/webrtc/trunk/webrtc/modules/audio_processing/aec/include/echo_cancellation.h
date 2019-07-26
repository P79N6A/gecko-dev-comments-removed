









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_INCLUDE_ECHO_CANCELLATION_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_INCLUDE_ECHO_CANCELLATION_H_

#include "typedefs.h"


#define AEC_UNSPECIFIED_ERROR           12000
#define AEC_UNSUPPORTED_FUNCTION_ERROR  12001
#define AEC_UNINITIALIZED_ERROR         12002
#define AEC_NULL_POINTER_ERROR          12003
#define AEC_BAD_PARAMETER_ERROR         12004


#define AEC_BAD_PARAMETER_WARNING       12050

enum {
    kAecNlpConservative = 0,
    kAecNlpModerate,
    kAecNlpAggressive
};

enum {
    kAecFalse = 0,
    kAecTrue
};

typedef struct {
    WebRtc_Word16 nlpMode;        
    WebRtc_Word16 skewMode;       
    WebRtc_Word16 metricsMode;    
    int delay_logging;            
    
} AecConfig;

typedef struct {
    WebRtc_Word16 instant;
    WebRtc_Word16 average;
    WebRtc_Word16 max;
    WebRtc_Word16 min;
} AecLevel;

typedef struct {
    AecLevel rerl;
    AecLevel erl;
    AecLevel erle;
    AecLevel aNlp;
} AecMetrics;

#ifdef __cplusplus
extern "C" {
#endif















WebRtc_Word32 WebRtcAec_Create(void **aecInst);













WebRtc_Word32 WebRtcAec_Free(void *aecInst);















WebRtc_Word32 WebRtcAec_Init(void *aecInst,
                             WebRtc_Word32 sampFreq,
                             WebRtc_Word32 scSampFreq);
















WebRtc_Word32 WebRtcAec_BufferFarend(void *aecInst,
                                     const WebRtc_Word16 *farend,
                                     WebRtc_Word16 nrOfSamples);



























WebRtc_Word32 WebRtcAec_Process(void *aecInst,
                                const WebRtc_Word16 *nearend,
                                const WebRtc_Word16 *nearendH,
                                WebRtc_Word16 *out,
                                WebRtc_Word16 *outH,
                                WebRtc_Word16 nrOfSamples,
                                WebRtc_Word16 msInSndCardBuf,
                                WebRtc_Word32 skew);















WebRtc_Word32 WebRtcAec_set_config(void *aecInst, AecConfig config);















WebRtc_Word32 WebRtcAec_get_config(void *aecInst, AecConfig *config);















WebRtc_Word32 WebRtcAec_get_echo_status(void *aecInst, WebRtc_Word16 *status);















WebRtc_Word32 WebRtcAec_GetMetrics(void *aecInst, AecMetrics *metrics);
















int WebRtcAec_GetDelayMetrics(void* handle, int* median, int* std);












WebRtc_Word32 WebRtcAec_get_error_code(void *aecInst);

#ifdef __cplusplus
}
#endif
#endif
