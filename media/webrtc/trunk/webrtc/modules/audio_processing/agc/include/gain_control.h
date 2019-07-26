









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AGC_INCLUDE_GAIN_CONTROL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AGC_INCLUDE_GAIN_CONTROL_H_

#include "typedefs.h"


#define AGC_UNSPECIFIED_ERROR           18000
#define AGC_UNSUPPORTED_FUNCTION_ERROR  18001
#define AGC_UNINITIALIZED_ERROR         18002
#define AGC_NULL_POINTER_ERROR          18003
#define AGC_BAD_PARAMETER_ERROR         18004


#define AGC_BAD_PARAMETER_WARNING       18050

enum
{
    kAgcModeUnchanged,
    kAgcModeAdaptiveAnalog,
    kAgcModeAdaptiveDigital,
    kAgcModeFixedDigital
};

enum
{
    kAgcFalse = 0,
    kAgcTrue
};

typedef struct
{
    WebRtc_Word16 targetLevelDbfs;   
    WebRtc_Word16 compressionGaindB; 
    WebRtc_UWord8 limiterEnable;     
} WebRtcAgc_config_t;

#if defined(__cplusplus)
extern "C"
{
#endif
















int WebRtcAgc_AddFarend(void* agcInst,
                        const WebRtc_Word16* inFar,
                        WebRtc_Word16 samples);
























int WebRtcAgc_AddMic(void* agcInst,
                     WebRtc_Word16* inMic,
                     WebRtc_Word16* inMic_H,
                     WebRtc_Word16 samples);



























int WebRtcAgc_VirtualMic(void* agcInst,
                         WebRtc_Word16* inMic,
                         WebRtc_Word16* inMic_H,
                         WebRtc_Word16 samples,
                         WebRtc_Word32 micLevelIn,
                         WebRtc_Word32* micLevelOut);







































int WebRtcAgc_Process(void* agcInst,
                      const WebRtc_Word16* inNear,
                      const WebRtc_Word16* inNear_H,
                      WebRtc_Word16 samples,
                      WebRtc_Word16* out,
                      WebRtc_Word16* out_H,
                      WebRtc_Word32 inMicLevel,
                      WebRtc_Word32* outMicLevel,
                      WebRtc_Word16 echo,
                      WebRtc_UWord8* saturationWarning);















int WebRtcAgc_set_config(void* agcInst, WebRtcAgc_config_t config);















int WebRtcAgc_get_config(void* agcInst, WebRtcAgc_config_t* config);








int WebRtcAgc_Create(void **agcInst);










int WebRtcAgc_Free(void *agcInst);

















int WebRtcAgc_Init(void *agcInst,
                   WebRtc_Word32 minLevel,
                   WebRtc_Word32 maxLevel,
                   WebRtc_Word16 agcMode,
                   WebRtc_UWord32 fs);

#if defined(__cplusplus)
}
#endif

#endif
