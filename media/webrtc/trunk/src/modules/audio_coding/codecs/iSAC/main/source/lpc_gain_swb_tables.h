

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_GAIN_SWB_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_GAIN_SWB_TABLES_H_

#include "settings.h"
#include "typedefs.h"

extern const double WebRtcIsac_kQSizeLpcGain;

extern const double WebRtcIsac_kLeftRecPointLpcGain[SUBFRAMES];

extern const WebRtc_Word16 WebRtcIsac_kNumQCellLpcGain[SUBFRAMES];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainEntropySearch[SUBFRAMES];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainCdfVec0[18];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainCdfVec1[21];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainCdfVec2[26];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainCdfVec3[46];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainCdfVec4[78];

extern const WebRtc_UWord16 WebRtcIsac_kLpcGainCdfVec5[171];

extern const WebRtc_UWord16* WebRtcIsac_kLpcGainCdfMat[SUBFRAMES];

extern const double WebRtcIsac_kLpcGainDecorrMat[SUBFRAMES][SUBFRAMES];

#endif 
