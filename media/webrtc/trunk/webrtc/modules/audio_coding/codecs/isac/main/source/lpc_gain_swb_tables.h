

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_GAIN_SWB_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_GAIN_SWB_TABLES_H_

#include "settings.h"
#include "typedefs.h"

extern const double WebRtcIsac_kQSizeLpcGain;

extern const double WebRtcIsac_kLeftRecPointLpcGain[SUBFRAMES];

extern const int16_t WebRtcIsac_kNumQCellLpcGain[SUBFRAMES];

extern const uint16_t WebRtcIsac_kLpcGainEntropySearch[SUBFRAMES];

extern const uint16_t WebRtcIsac_kLpcGainCdfVec0[18];

extern const uint16_t WebRtcIsac_kLpcGainCdfVec1[21];

extern const uint16_t WebRtcIsac_kLpcGainCdfVec2[26];

extern const uint16_t WebRtcIsac_kLpcGainCdfVec3[46];

extern const uint16_t WebRtcIsac_kLpcGainCdfVec4[78];

extern const uint16_t WebRtcIsac_kLpcGainCdfVec5[171];

extern const uint16_t* WebRtcIsac_kLpcGainCdfMat[SUBFRAMES];

extern const double WebRtcIsac_kLpcGainDecorrMat[SUBFRAMES][SUBFRAMES];

#endif 
