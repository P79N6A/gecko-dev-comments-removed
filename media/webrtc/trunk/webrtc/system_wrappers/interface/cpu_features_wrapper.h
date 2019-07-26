









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_FEATURES_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_FEATURES_WRAPPER_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <typedefs.h>


typedef enum {
  kSSE2,
  kSSE3
} CPUFeature;


enum {
  kCPUFeatureARMv7       = (1 << 0),
  kCPUFeatureVFPv3       = (1 << 1),
  kCPUFeatureNEON        = (1 << 2),
  kCPUFeatureLDREXSTREX  = (1 << 3)
};

typedef int (*WebRtc_CPUInfo)(CPUFeature feature);


extern WebRtc_CPUInfo WebRtc_GetCPUInfo;


extern WebRtc_CPUInfo WebRtc_GetCPUInfoNoASM;




extern uint64_t WebRtc_GetCPUFeaturesARM(void);

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
