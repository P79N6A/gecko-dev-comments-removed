









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_INCLUDE_ECHO_CANCELLATION_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_INCLUDE_ECHO_CANCELLATION_H_

#include "webrtc/typedefs.h"


#define AEC_UNSPECIFIED_ERROR 12000
#define AEC_UNSUPPORTED_FUNCTION_ERROR 12001
#define AEC_UNINITIALIZED_ERROR 12002
#define AEC_NULL_POINTER_ERROR 12003
#define AEC_BAD_PARAMETER_ERROR 12004


#define AEC_BAD_PARAMETER_WARNING 12050

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
  int16_t nlpMode;      
  int16_t skewMode;     
  int16_t metricsMode;  
  int delay_logging;    
  
} AecConfig;

typedef struct {
  int instant;
  int average;
  int max;
  int min;
} AecLevel;

typedef struct {
  AecLevel rerl;
  AecLevel erl;
  AecLevel erle;
  AecLevel aNlp;
} AecMetrics;

struct AecCore;

#ifdef __cplusplus
extern "C" {
#endif















int32_t WebRtcAec_Create(void** aecInst);













int32_t WebRtcAec_Free(void* aecInst);















int32_t WebRtcAec_Init(void* aecInst, int32_t sampFreq, int32_t scSampFreq);
















int32_t WebRtcAec_BufferFarend(void* aecInst,
                               const int16_t* farend,
                               int16_t nrOfSamples);



























int32_t WebRtcAec_Process(void* aecInst,
                          const int16_t* nearend,
                          const int16_t* nearendH,
                          int16_t* out,
                          int16_t* outH,
                          int16_t nrOfSamples,
                          int16_t msInSndCardBuf,
                          int32_t skew);















int WebRtcAec_set_config(void* handle, AecConfig config);















int WebRtcAec_get_echo_status(void* handle, int* status);















int WebRtcAec_GetMetrics(void* handle, AecMetrics* metrics);
















int WebRtcAec_GetDelayMetrics(void* handle, int* median, int* std);












int32_t WebRtcAec_get_error_code(void* aecInst);









struct AecCore* WebRtcAec_aec_core(void* handle);

#ifdef __cplusplus
}
#endif
#endif
