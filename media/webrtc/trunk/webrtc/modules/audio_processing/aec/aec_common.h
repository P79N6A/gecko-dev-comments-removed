









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_COMMON_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_COMMON_H_

#include "webrtc/typedefs.h"

#ifdef _MSC_VER 
#define ALIGN16_BEG __declspec(align(16))
#define ALIGN16_END
#else 
#define ALIGN16_BEG
#define ALIGN16_END __attribute__((aligned(16)))
#endif

extern ALIGN16_BEG const float ALIGN16_END WebRtcAec_sqrtHanning[65];
extern ALIGN16_BEG const float ALIGN16_END WebRtcAec_weightCurve[65];
extern ALIGN16_BEG const float ALIGN16_END WebRtcAec_overDriveCurve[65];
extern const float WebRtcAec_kExtendedSmoothingCoefficients[2][2];
extern const float WebRtcAec_kNormalSmoothingCoefficients[2][2];
extern const float WebRtcAec_kMinFarendPSD;

#endif  

