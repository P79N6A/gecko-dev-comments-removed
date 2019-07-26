









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_TRANSFORM_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_TRANSFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "webrtc/modules/audio_coding/codecs/isac/fix/source/settings.h"
#include "webrtc/typedefs.h"


extern const int16_t kCosTab1[FRAMESAMPLES/2];


extern const int16_t kSinTab1[FRAMESAMPLES/2];


extern const int16_t kCosTab2[FRAMESAMPLES/4];


extern const int16_t kSinTab2[FRAMESAMPLES/4];

#ifdef __cplusplus
} 
#endif

#endif
